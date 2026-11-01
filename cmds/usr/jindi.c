// secret_realm.c
// 秘境试炼玩家命令（#108 c7：血色禁地真实进入/退出/探索）
// 用法：jindi                - 查看可用秘境
//       jindi enter          - 进入血色禁地（校验开放状态与境界门槛）
//       jindi exit           - 退出当前秘境（结算并送回入口）
//       jindi explore        - 探索当前所在层（生成真实妖兽并开战）
//       jindi info           - 查看当前实例状态
//
// 入口为真实房间 /d/xuejin/rukou（越国七派传送阵南侧），进入需筑基中期
// （#61 境界体系，口径见 .knowledge/design/secret-realm-entry-standard.md）。
// 进入后沿房间行走（外围→中层→核心→巢窟），探索接真实战斗（#63 战斗系统）。

#include <ansi.h>
#include <secret_realm.h>
#include <globals.h>

int help(object me);

// 主入口
int main(object me, string arg)
{
    object srd;
    string realm_id = "blood_forbidden";

    // 确保守护进程已加载
    srd = find_object(SECRET_REALM_D);
    if (!srd)
    {
        srd = load_object(SECRET_REALM_D);
        if (!srd)
            return notify_fail("秘境系统暂不可用，请稍后再试。\n");
    }

    if (!arg || arg == "")
    {
        // 无参数：列出可用秘境
        mapping realm;
        int status;
        write(HIC "════════ 秘境试炼 ════════\n" NOR);
        realm = srd->query_realm(realm_id);
        if (mapp(realm))
        {
            status = srd->query_realm_status(realm_id);
            write("  " + realm[SR_FIELD_NAME] +
                  (status == SR_STATUS_OPEN ? " " HIG "（入口开放）" NOR :
                   " " HIR "（入口封闭）" NOR) + "\n");
            write("  要求：筑基中期及以上\n");
            write("  进入：jindi enter    退出：jindi exit    探索：jindi explore\n");
        }
        else
        {
            write("  暂无可进入的秘境。\n");
        }
        return 1;
    }

    // ── enter：进入血色禁地 ──
    if (arg == "enter")
    {
        mapping realm;
        string inst_id;

        // 已有活跃实例则不重复创建
        inst_id = srd->query_player_instance(me, realm_id);
        if (stringp(inst_id) && inst_id != "")
        {
            write("你已身处试炼之中。输入 jindi explore 探索，jindi exit 退出。\n");
            return 1;
        }

        realm = srd->query_realm(realm_id);
        if (!mapp(realm))
        {
            write("该秘境不存在。\n");
            return 1;
        }

        // 开放状态 + 境界门槛校验（#61 境界体系，筑基中期起）
        if (!srd->check_entry_condition(me, realm_id))
        {
            write(srd->entry_fail_reason(me, realm_id));
            return 1;
        }

        inst_id = srd->create_instance(me, realm_id);
        if (!stringp(inst_id) || inst_id == "")
        {
            write("秘境入口的禁制波动了一下，你没能进入。\n");
            return 1;
        }

        me->move(realm[SR_FIELD_ENTRY]);
        write(HIG "血雾在身后合拢，你踏入了" + realm[SR_FIELD_NAME] + "。\n" NOR);
        write("四周毒雾弥漫，北面的密林深处传来妖兽的嘶鸣。\n");
        write("提示：jindi explore 探索本层；jindi exit 退出试炼。\n");
        return 1;
    }

    // ── exit：退出当前秘境 ──
    if (arg == "exit")
    {
        string room_name;

        room_name = base_name(environment(me));
        if (!stringp(room_name) || strsrch(room_name, "/d/xuejin/") != 0)
        {
            write("你并不在血色禁地之中。\n");
            return 1;
        }

        write(srd->do_exit(me, realm_id));
        return 1;
    }

    // ── explore：探索当前所在层（接真实战斗） ──
    if (arg == "explore")
    {
        string inst_id;

        inst_id = srd->query_player_instance(me, realm_id);
        if (!stringp(inst_id) || inst_id == "")
        {
            write("你还没有进入血色禁地。输入 jindi enter 进入试炼。\n");
            return 1;
        }

        write(srd->do_explore(me, inst_id));
        return 1;
    }

    // ── info：查看当前实例状态 ──
    if (arg == "info")
    {
        string inst_id;

        inst_id = srd->query_player_instance(me, realm_id);
        if (!stringp(inst_id) || inst_id == "")
        {
            write("你还没有进入血色禁地。\n");
            return 1;
        }

        write(srd->query_layer_info(me, inst_id));
        return 1;
    }

    // 未知参数
    write("用法：jindi            - 查看可用秘境\n");
    write("      jindi enter      - 进入血色禁地（需筑基中期）\n");
    write("      jindi exit       - 退出试炼（结算并送回入口）\n");
    write("      jindi explore    - 探索当前所在层（开战）\n");
    write("      jindi info       - 查看当前实例状态\n");
    return 1;
}

int help(object me)
{
    write(@HELP
指令：jindi（血色禁地试炼）

用法：
  jindi            - 查看可用秘境
  jindi enter      - 进入血色禁地（需筑基中期及以上，入口开放时）
  jindi exit       - 退出试炼，结算奖励并送回入口
  jindi explore    - 探索当前所在层：生成本层真实妖兽并开战
  jindi info       - 查看当前试炼实例状态

说明：
- 血色禁地为三层环形结构：外围迷瘴森林 → 中层遗迹广场 → 核心地下洞窟 → 墨蛟巢窟
- 进入后沿房间方向行走即可在各层之间往返，随时可返回入口
- 探索触发的战斗为真实战斗（攻击/法术/逃跑均有效）
- 击败巢窟深处的墨蛟并活着离开，即「血禁试炼存活」，各派据此破格擢升
HELP
    );
    return 1;
}
