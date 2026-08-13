// ai.c
// AI 自然语言辅助命令（#70 Phase 1）
//
// 用法：
//   ai <自然语言>      —— 把自然语言翻译为 MUD 指令序列并异步注入执行
//   ai confirm yes     —— 确认执行待确认的高危指令（上一条 ai 请求的 confirm 队列）
//   ai confirm no      —— 取消待确认的高危指令
//   ai help            —— 帮助
//
// 开关（双闸）：
//   全局：LLM_D 默认关闭（管理员 set_llm_enabled(1) 开启）
//   玩家：set llm on（env/llm opt-in，复用 set.c 通用 env 机制）
// 请求经 LLM_D（adm/daemons/llmd.c）走 TCP 127.0.0.1 连本地 sidecar
// （tools/llm/sidecar.py）；异步返回，watchdog 15s 超时零注入。

#include <ansi.h>
#include <globals.h>

inherit F_CLEAN_UP;

int help(object me);

int main(object me, string arg)
{
	string text;

	if (me != this_player(1)) return 0;
	if (!stringp(arg) || arg == "" || arg == "help")
		return help(me);

	// 全局开关（管理员控制，默认关闭）
	if (!LLM_D->is_llm_enabled())
		return notify_fail("AI 辅助功能当前未开放。\n");

	// 玩家 opt-in（set llm on）
	if (!LLM_D->player_opted_in(me))
		return notify_fail("请先使用「set llm on」开启 AI 辅助。\n");

	// 高危确认分支
	if (sscanf(arg, "confirm %s", text) == 1)
	{
		text = lower_case(text);
		if (text == "yes" || text == "y")
			return LLM_D->confirm_pending(me, 1);
		if (text == "no" || text == "n")
			return LLM_D->confirm_pending(me, 0);
		return notify_fail("格式：ai confirm yes（执行）或 ai confirm no（取消）。\n");
	}
	// 裸 confirm（漏了 yes/no）→ 提示，不要把 "confirm" 当自然语言发给 sidecar
	if (arg == "confirm")
		return notify_fail("格式：ai confirm yes（执行待确认高危指令）或 ai confirm no（取消）。\n");

	tell_object(me, HIG "正在理解你的意图，请稍候……\n" NOR);
	LLM_D->request_parse(me, arg);
	return 1;
}

int help(object me)
{
	write(@HELP
指令格式 : ai <自然语言> | ai confirm yes/no | ai help

把自然语言描述的意图翻译成 MUD 指令序列并执行。例如：

  ai 去当铺把金条当了
  ai 看看我背包里有什么

说明：
- 需要先开启：set llm on（全局开放后本命令才可用）
- 解析在本地 AI 服务（sidecar）完成，LLM API 密钥由玩家在自己主机上
  配置（环境变量或 ~/.config/fanren-mud/llm.json），不进入游戏或代码库
- 安全：只执行白名单内的安全指令；危险指令（战斗/财产转移/退出等）会
  拦截并提示确认——输入 ai confirm yes 执行、ai confirm no 取消
- 管理/调试命令（shutdown/exec/update 等）一律拒绝
- 若 15 秒内无响应则超时中止，不注入任何指令
HELP
	);
	return 1;
}
