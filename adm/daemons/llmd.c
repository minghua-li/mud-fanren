// llmd.c
// LLM 自然语言解析 daemon（#70 Phase 1）
//
// 职责边界（架构评估 5276954767 口径）：
//   LPC 侧只做：采集 grounding 上下文 → TCP 127.0.0.1 转发本地 sidecar →
//               异步回调 → 逐条 force_me 注入 + 上下文变化校验 + 失败安全兜底；
//   NL→指令映射 / prompt 组装 / LLM 调用 / safety 过滤 全在 sidecar
//   （tools/llm/sidecar.py，复用 #69 llm_client.py / safety.py / mock_llm.py）。
//
// 协议：每请求一连接，新行分隔 JSON；sidecar 监听 127.0.0.1:LLM_SIDECAR_PORT。
// 请求 = 单行 JSON（player/language/grounding）；响应 = 单行 JSON
// （commands/confirm/blocked/reason/error）。
//
// 失败安全（延续 #69）：
//   - watchdog：请求发出后 LLM_WATCHDOG_TIME(15s) 未收到响应 → 关 fd、清 pending、
//     零注入（迟到响应因 fd 已关进不来；pending 已删即使进来也直接忽略）。
//   - json_parse 失败 / sidecar 未运行 / 连接失败 / 响应格式非法 → 一律零注入。
//   - LPC 侧对注入前每条指令再做字符集校验（防注入最后防线，
//     对齐 safety.py 的 [a-z][a-z0-9_-] + 空格字符集）。
//
// 开关：全局默认关闭（query("llm_enabled") == 0，管理员 set_llm_enabled(1) 开启）；
//       玩家侧还需 set llm on（env/llm）opt-in，双闸都开才接受 ai 请求。

#include <ansi.h>
#include <net/socket.h>
#include <socket_err.h>
#include <globals.h>

inherit F_DBASE;

// 每玩家最多 1 个未决请求（防单玩家并发刷请求打满 socket）
#define MAX_PENDING_PER_PLAYER  1
// 注入指令的最大条数（sidecar 已限 MAX_COMMANDS=8，此处为 LPC 侧兜底上限）
#define MAX_INJECT_COMMANDS     8
// 合法指令字符集（与 safety.py _format_check 一致）
#define CMD_LEGAL_CHARS "abcdefghijklmnopqrstuvwxyz0123456789_- "

// LPC 侧动词白名单（防御纵深：即使 sidecar 被替换/配置错误，LPC 也只注入
// 白名单指令。词表与 tools/llm/safety.py 的 SAFE_VERBS/DANGEROUS_VERBS/
// DENIED_VERBS 保持一致——跨语言复制，改 safety.py 时须同步）
#define LLM_SAFE_VERBS ({ "go","look","l","inventory","i","score","hp","check", \
        "list","value","pawn","dang","buy","sell","get","put","open","close", \
        "wear","wield","unwield","remove","say","tell","reply","smile","nod", \
        "ask","watch","listen","sit","sleep","eat","drink" })
#define LLM_DANGEROUS_VERBS ({ "kill","hit","fight","steal","attack","give", \
        "drop","discard","quit","exit","relogin","suicide","reset","destruct" })
#define LLM_DENIED_VERBS ({ "shutdown","exec","call","debug","update","reload", \
        "clone","cd","pwd","ls","rm","mv","cp","tail","cat","ed","more", \
        "mkdir","rmdir","chmod","patch","diff","grep","snoop","chinese", \
        "adm","arch","wiz","immortal","eval","efun","dump","profile","mem", \
        "netstat","ps" })

// pending 表：fd → ([ "player": object, "payload": string, "buf": string, "sent": int ])
protected mapping pending = ([]);

// ── 前向声明 ────────────────────────────────────────────
void read_callback(int fd, mixed message);
protected void close_callback(int fd);
void write_callback(int fd);
void watchdog_timeout(int fd);
protected void handle_response(int fd, string line);
protected void cleanup_fd(int fd);
string collect_grounding(object me, string text);
int valid_inject_cmd(string cmd, int mode);
void inject_commands(object me, string *cmds, int mode);

void create()
{
	seteuid(ROOT_UID);
	set("name", "LLM 解析系统");
	set("id", "llm_d");
	// 全局开关默认关闭：管理员显式 set_llm_enabled(1) 后才接受 ai 请求；
	// 玩家侧还需 set llm on（env/llm）opt-in，双闸都开才可用。
	set("llm_enabled", 0);
}

// ── 开关 ────────────────────────────────────────────────
int is_llm_enabled() { return query("llm_enabled"); }
void set_llm_enabled(int on) { set("llm_enabled", on ? 1 : 0); }

// 玩家是否已 opt-in（set llm on → env/llm；兼容 "on"/"1"/1/yes/true 写法）
int player_opted_in(object me)
{
	mixed v;

	if (!objectp(me)) return 0;
	v = me->query("env/llm");
	if (undefinedp(v)) return 0;
	if (intp(v)) return v == 1;
	if (stringp(v))
	{
		v = lower_case(v);
		return v == "on" || v == "yes" || v == "1" || v == "true";
	}
	return 0;
}

// 该玩家当前未决请求数（限流）
int player_pending_count(object me)
{
	int n;

	n = 0;
	foreach (int fd, mapping m in pending)
		if (m["player"] == me)
			n++;
	return n;
}

// ── grounding 采集 ──────────────────────────────────────
string collect_grounding(object me, string text)
{
	mapping ctx;
	object env;
	object *inv;
	string *names;
	int i;

	ctx = ([]);
	env = environment(me);
	if (env)
	{
		ctx["room_short"] = env->query("short");
		ctx["room_long"] = env->query("long");
		if (mapp(env->query("exits")))
			ctx["exits"] = keys(env->query("exits"));
		names = ({});
		inv = all_inventory(env);
		for (i = 0; i < sizeof(inv); i++)
			if (inv[i] != me && inv[i]->query("name"))
				names += ({ inv[i]->query("name") });
		ctx["objects"] = names;
	}

	names = ({});
	inv = all_inventory(me);
	for (i = 0; i < sizeof(inv); i++)
		if (inv[i]->query("name"))
			names += ({ inv[i]->query("name") });
	ctx["inventory"] = names;

	// 境界（#61 ROOT_REFINE_D）；daemon 未加载或调用异常时安全兜底为空
	catch(ctx["realm"] = ROOT_REFINE_D->query_player_realm(me));

	return json_encode(([
		"player" : me->query("id"),
		"language" : text,
		"grounding" : ctx,
	]));
}

// ── 请求入口（ai.c 调用）────────────────────────────────
varargs int request_parse(object me, string text)
{
	int fd;
	int err;
	string payload;
	mapping m;

	if (!objectp(me) || !stringp(text) || text == "")
		return 0;
	if (!is_llm_enabled())
		return 0;
	if (!player_opted_in(me))
		return 0;
	if (player_pending_count(me) >= MAX_PENDING_PER_PLAYER)
	{
		tell_object(me, "你已有一个 AI 请求在处理中，请稍候。\n");
		return 0;
	}

	fd = socket_create(STREAM, "read_callback", "close_callback");
	if (fd < 0)
	{
		tell_object(me, "AI 服务初始化失败（socket 创建失败）。\n");
		return 0;
	}

	payload = collect_grounding(me, text);
	m = ([]);
	m["player"] = me;
	m["payload"] = payload;
	m["buf"] = "";
	m["sent"] = 0;
	pending[fd] = m;

	err = socket_connect(fd, sprintf("%s %d", LLM_SIDECAR_HOST, LLM_SIDECAR_PORT),
	                     "read_callback", "write_callback");
	if (err != EESUCCESS)
	{
		// sidecar 未运行 / 连接被拒 → 立即失败，零注入
		tell_object(me, "AI 服务连接失败（sidecar 未运行？），本次请求已安全中止。\n");
		cleanup_fd(fd);
		return 0;
	}
	return 1;
}

// ── socket 回调 ─────────────────────────────────────────
void write_callback(int fd)
{
	mapping m;
	int err;
	int h;

	if (!mapp(pending[fd])) return;
	m = pending[fd];
	if (m["sent"]) return;
	m["sent"] = 1;
	pending[fd] = m;

	err = socket_write(fd, m["payload"] + "\n");
	if (err != EESUCCESS)
	{
		// 写入失败（连接异常）→ 零注入
		if (objectp(m["player"]))
			tell_object(m["player"], "AI 服务通信失败，本次请求已安全中止。\n");
		cleanup_fd(fd);
		return;
	}
	// watchdog：LLM_WATCHDOG_TIME 秒未收到响应 → 超时清理、零注入
	// （handle 存表备用但刻意不 remove_call_out：LPC 的 remove_call_out 按
	//   函数名整体移除（见 economyd.c:62 先例），会误伤其他玩家同函数的挂起；
	//   靠「响应到达即删 pending，watchdog 触发时查表为空则空跑」精确失效，单线程安全）
	h = call_out("watchdog_timeout", LLM_WATCHDOG_TIME, fd);
	m["watchdog"] = h;
	pending[fd] = m;
}

void read_callback(int fd, mixed message)
{
	mapping m;
	string buf;
	int nl;

	if (!mapp(pending[fd])) return;  // 已超时清理 → 迟到数据，零注入
	m = pending[fd];
	if (stringp(m["buf"]))
		buf = m["buf"] + message;
	else
		buf = message;

	// 单行 JSON 响应，\n 定界；未收完整行则缓冲等下次回调
	nl = strsrch(buf, "\n");
	if (nl == -1)
	{
		m["buf"] = buf;
		pending[fd] = m;
		return;
	}
	handle_response(fd, buf[0..nl - 1]);
}

void close_callback(int fd)
{
	mapping m;

	// 连接关闭：若仍有未决请求（未响应也未超时）→ 视为异常中断，零注入
	// 若 pending 已删（cleanup_fd 自身触发的回调）→ 直接返回，防重入
	if (!mapp(pending[fd])) return;
	m = pending[fd];
	if (!m["done"] && objectp(m["player"]))
		tell_object(m["player"], "AI 服务连接意外中断，本次请求已安全中止（未注入任何指令）。\n");
	cleanup_fd(fd);
}

// ── watchdog ────────────────────────────────────────────
void watchdog_timeout(int fd)
{
	mapping m;

	if (!mapp(pending[fd])) return;  // 响应已处理 → 空跑
	m = pending[fd];
	if (objectp(m["player"]))
		tell_object(m["player"], sprintf(HIY "AI 解析超时（%d 秒未响应），已安全中止，未注入任何指令。\n" NOR,
		                                LLM_WATCHDOG_TIME));
	cleanup_fd(fd);
}

// ── 响应处理 ────────────────────────────────────────────
protected void handle_response(int fd, string line)
{
	mapping m;
	mixed data;
	mixed err;
	string *cmds;
	string *confirms;
	string *blocked;
	string reason;
	string err_msg;

	if (!mapp(pending[fd])) return;
	m = pending[fd];

	// 解析单行 JSON（json_parse efun，FluffOS v2019 内置；失败 → 零注入）
	err = catch(data = json_parse(line));
	if (err || !mapp(data))
	{
		if (objectp(m["player"]))
			tell_object(m["player"], "AI 响应格式非法，本次请求已安全中止（未注入任何指令）。\n");
		cleanup_fd(fd);
		return;
	}

	// error 字段非空 = sidecar 解析失败（LLM 失败/超时/未配 key/请求非法）
	err_msg = data["error"];
	if (stringp(err_msg) && err_msg != "")
	{
		if (objectp(m["player"]))
			tell_object(m["player"], "AI 解析失败：" + err_msg + "（未注入任何指令）。\n");
		cleanup_fd(fd);
		return;
	}

	// blocked 展示（拦截原因）
	blocked = data["blocked"];
	if (pointerp(blocked) && sizeof(blocked) && objectp(m["player"]))
		tell_object(m["player"], "已拦截 " + sizeof(blocked) + " 条不安全指令：\n" +
		                          implode(blocked, "\n") + "\n");

	reason = data["reason"];
	if (stringp(reason) && reason != "" && objectp(m["player"]))
		tell_object(m["player"], "AI 意图解析：" + reason + "\n");

	// commands：直接注入（上下文变化校验 + 白名单/字符集最后防线）
	cmds = data["commands"];
	if (pointerp(cmds) && sizeof(cmds) && objectp(m["player"]))
		inject_commands(m["player"], cmds, 0);

	// confirm：高危意图 → 交给玩家二次确认（存玩家 temp，ai confirm yes/no）
	confirms = data["confirm"];
	if (pointerp(confirms) && sizeof(confirms) && objectp(m["player"]))
	{
		m["player"]->set_temp("llm_confirm", confirms);
		tell_object(m["player"], HIW "以下高危指令需要确认才能执行：\n" NOR +
		                          implode(confirms, "\n") + "\n" +
		                          HIG "输入 ai confirm yes 全部执行，或 ai confirm no 取消。\n" NOR);
	}

	cleanup_fd(fd);
}

// 玩家二次确认（ai.c：ai confirm yes / ai confirm no）
int confirm_pending(object me, int yes)
{
	string *confirm_cmds;

	if (!objectp(me)) return 0;
	confirm_cmds = me->query_temp("llm_confirm");
	me->delete_temp("llm_confirm");

	if (!yes)
	{
		if (pointerp(confirm_cmds) && sizeof(confirm_cmds))
			tell_object(me, "已取消高危指令执行。\n");
		else
			tell_object(me, "当前没有待确认的高危指令。\n");
		return 1;
	}
	if (!pointerp(confirm_cmds) || !sizeof(confirm_cmds))
	{
		tell_object(me, "当前没有待确认的高危指令。\n");
		return 1;
	}
	tell_object(me, "已确认，执行高危指令。\n");
	inject_commands(me, confirm_cmds, 1);
	return 1;
}

// ── 指令注入 ────────────────────────────────────────────
// mode 0 = 安全注入（动词须在白名单 SAFE）；mode 1 = confirm 注入（仅 DANGEROUS 可放行）
int valid_inject_cmd(string cmd, int mode)
{
	int i;
	string c;
	string verb;

	if (!stringp(cmd) || cmd == "" || strlen(cmd) > 80)
		return 0;
	for (i = 0; i < strlen(cmd); i++)
	{
		c = cmd[i..i];
		if (strsrch(CMD_LEGAL_CHARS, c) == -1)
			return 0;
	}
	verb = explode(cmd, " ")[0];
	if (member_array(verb, LLM_DENIED_VERBS) != -1)
		return 0;                      // 管理/调试命令绝对拒（wiz 指令被拒）
	if (mode == 0)
		return member_array(verb, LLM_SAFE_VERBS) != -1;
	return member_array(verb, LLM_DANGEROUS_VERBS) != -1;
}

void inject_commands(object me, string *cmds, int mode)
{
	int i;

	if (!objectp(me) || !living(me)) return;  // 玩家不在 → 零注入
	for (i = 0; i < sizeof(cmds) && i < MAX_INJECT_COMMANDS; i++)
	{
		// 上下文变化校验：玩家死亡/离线/离开 → 中止后续（目标对象已消失等）
		if (!objectp(me) || !living(me)) break;
		if (!valid_inject_cmd(cmds[i], mode))
		{
			tell_object(me, "已拦截一条不合法指令：" + cmds[i] + "\n");
			continue;
		}
		me->force_me(cmds[i]);
	}
}

// ── 清理 ────────────────────────────────────────────────
protected void cleanup_fd(int fd)
{
	mapping m;

	if (mapp(pending[fd]))
	{
		m = pending[fd];
		m["done"] = 1;
		pending[fd] = m;
		// 先删表再关连接：socket_close 触发 close_callback 时 pending 已无此 fd，
		// 回调直接返回，防重入/重复 close
		map_delete(pending, fd);
	}
	socket_close(fd);
}
