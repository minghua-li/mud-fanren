#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""#64 声望商店购买交付 + 商品目录清理（shop_d）静态守卫（可复跑验证）
运行：python3 tools/check/shop_verify.py
覆盖：
  一、目录完整性：解析 shop_d.c item_catalog 全部对象路径，断言逐条 .c 源文件真实存在
      （零 MISS）；/obj/weapon、/obj/armor、/obj/item、/obj/skill 四个死目录引用不得回潮
  二、购买交付原文守卫：buy_item 扣费前 new(item[3]) 预生成、失败返回 -6 且不扣费、
      扣费后 move(player) 交付、交付失败回滚；query_available_items 用 file_exists
      过滤源文件不存在的条目
  三、突变实证：把一条目录改回死路径 /obj/weapon/* → 守卫红
  四、购买失败两类拆分契约：buy_item 存在 return -7（交付失败）、faction.c 存在
      case -7，且 -6/-7 两条提示分别含「商品数据异常」/「背包无法容纳」（均明示
      「未扣除任何费用」）
  五、突变实证：把 -7 改回 -6（shop_d.c 与 faction.c 两处）→ 守卫必须红
  六、#105 退款专用裸回补契约：reputation_d.c 存在 refund_reputation
      (object player, string faction, int amount, string reason)，函数体不触
      每日上限/势力互斥、强制写审计日志且强制 reason；buy_item 回滚分支改用
      该接口、不再出现 add_reputation
  七、突变实证：把 buy_item 回滚改回 add_reputation（普通加声望）→ 守卫必须红
退出码：0 全绿，1 有失败
"""
import os
import re
import shutil
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.chdir(ROOT)

oks = 0
fails = 0


def ok(msg):
    global oks
    oks += 1
    print('  ok   %s' % msg)


def err(msg):
    global fails
    fails += 1
    print('  FAIL %s' % msg)


def readf(path):
    with open(path, encoding='utf-8') as f:
        return f.read()


def catalog_paths(src):
    """解析 item_catalog 块内的全部对象路径（以 / 开头的字符串字面量）"""
    m = re.search(r'item_catalog\s*=\s*\(\{', src)
    if not m:
        return None
    block = src[m.end():]
    end = block.find('});')
    if end == -1:
        return None
    return re.findall(r'"(/[^"]+)"', block[:end])


def guard(src, label='shop_d'):
    """返回失败消息列表（空 = 绿）。对任意 shop_d.c 原文可重跑"""
    bad = []
    paths = catalog_paths(src)
    if paths is None:
        bad.append('%s: 未找到 item_catalog 块' % label)
        return bad
    if not paths:
        bad.append('%s: item_catalog 未解析到任何对象路径' % label)
        return bad
    missing = [p for p in paths if not os.path.isfile(p.lstrip('/') + '.c')]
    if missing:
        bad.append('%s: %d 条对象路径不存在: %s' % (label, len(missing), sorted(set(missing))))
    dead = [p for p in paths
            if p.startswith(('/obj/weapon', '/obj/armor', '/obj/item', '/obj/skill'))]
    if dead:
        bad.append('%s: 死目录引用回潮: %s' % (label, sorted(set(dead))))
    return bad


def extract_func(src, name):
    """粗略截取函数体：从首次 `name(` 起、花括号配平到第一个闭括号"""
    i = src.find('%s(' % name)
    if i == -1:
        return ''
    j = src.find('{', i)
    if j == -1:
        return ''
    depth = 0
    for k in range(j, len(src)):
        if src[k] == '{':
            depth += 1
        elif src[k] == '}':
            depth -= 1
            if depth == 0:
                return src[i:k + 1]
    return src[i:]


def case_block(src, code):
    """截取 switch 内某返回码的 case 块：自 case <code>: 起，到下一个 case/default 止"""
    i = src.find('case %d:' % code)
    if i == -1:
        return None
    end = len(src)
    for token in ('case ', 'default:'):
        j = src.find(token, i + 1)
        if j != -1 and j < end:
            end = j
    return src[i:end]


def split_guard(shop_src, fac_src):
    """-6/-7 拆分契约：返回失败消息列表（空 = 绿）。对任意 shop_d.c / faction.c 原文可重跑"""
    bad = []
    buy = extract_func(shop_src, 'buy_item')
    if 'return -7' not in buy:
        bad.append('buy_item 缺少 return -7（交付失败分支丢失或被改回 -6）')
    if 'case -7:' not in fac_src:
        bad.append('faction.c 缺少 case -7 分支')
    b6 = case_block(fac_src, -6)
    b7 = case_block(fac_src, -7)
    if not (b6 and '商品数据异常' in b6):
        bad.append('faction.c case -6 提示缺「商品数据异常」')
    if not (b6 and '未扣除任何费用' in b6):
        bad.append('faction.c case -6 提示缺「未扣除任何费用」')
    if not (b7 and '背包无法容纳' in b7):
        bad.append('faction.c case -7 提示缺「背包无法容纳」')
    if not (b7 and '未扣除任何费用' in b7):
        bad.append('faction.c case -7 提示缺「未扣除任何费用」')
    return bad


def refund_guard(shop_src, rep_src):
    """#105 退款专用裸回补契约：返回失败消息列表（空 = 绿）。对任意原文可重跑。
    断言 refund_reputation 四参签名（reason 强制）、不触每日上限、不触势力互斥、
    强制写审计日志，且 buy_item 回滚分支改用该接口、不再调普通 add_reputation
    （后者受每日上限钳制并触发互斥副作用，正是本票要消灭的不对称）。"""
    bad = []
    buy = extract_func(shop_src, 'buy_item')
    ref = extract_func(rep_src, 'refund_reputation')
    if not ref:
        bad.append('reputation_d.c 缺少退款专用裸回补接口 refund_reputation')
    else:
        if not ref.startswith('refund_reputation(object player, string faction, int amount, string reason)'):
            bad.append('refund_reputation 签名不是 (object player, string faction, int amount, string reason)')
        if 'query_daily_cap' in ref:
            bad.append('refund_reputation 触发了每日声望上限（退款回补不应受限）')
        if 'apply_mutex' in ref:
            bad.append('refund_reputation 触发了势力互斥副作用（扣费侧不触发，回补侧必须对称不触发）')
        if 'log_file("reputation"' not in ref:
            bad.append('refund_reputation 未强制写审计日志')
        if '!stringp(reason)' not in ref and '!sizeof(reason)' not in ref:
            bad.append('refund_reputation 未强制 reason（审计留痕必需）')
    if 'refund_reputation(' not in buy:
        bad.append('buy_item 回滚分支未改用 refund_reputation 退款接口')
    if 'add_reputation(' in buy:
        bad.append('buy_item 仍在调普通 add_reputation（受每日上限钳制并触发互斥副作用，应改用 refund_reputation）')
    return bad


print('═══ 一、目录完整性（shop_d.c 对象路径） ═══')
shop = readf('adm/daemons/shop_d.c')
paths = catalog_paths(shop)
bad = guard(shop)
if not bad:
    ok('item_catalog %d 条对象路径全部真实存在（零 MISS）' % len(paths))
else:
    for b in bad:
        err(b)

print('\n═══ 二、购买交付原文守卫（绑定交付物原文） ═══')
buy = extract_func(shop, 'buy_item')
if 'new(item[3])' in buy:
    ok('buy_item 用 new(item[3]) 生成对象')
else:
    err('buy_item 未用 new(item[3]) 生成对象')

if buy and buy.find('new(item[3])') != -1 and buy.find('deduct_reputation') != -1 \
        and buy.find('new(item[3])') < buy.find('deduct_reputation'):
    ok('new() 预生成在扣费之前（生成失败不扣费）')
else:
    err('new() 预生成未先于扣费')

if 'return -6' in buy:
    ok('生成/交付失败返回 -6')
else:
    err('缺少 -6 返回值（生成失败分支丢失）')

if 'move(player)' in buy:
    ok('扣费后 move(player) 交付给玩家')
else:
    err('buy_item 未 move(player) 交付')

if 'refund_reputation(player, faction' in buy:
    ok('交付失败回滚用 refund_reputation 退款专用裸回补接口')
else:
    err('交付失败缺少回滚（应调 refund_reputation 裸回补）')

qai = extract_func(shop, 'query_available_items')
if 'file_exists' in qai:
    ok('query_available_items 用 file_exists 过滤源文件不存在的条目')
else:
    err('query_available_items 未过滤源文件不存在的条目')

print('\n═══ 三、突变实证（守卫绑定交付物） ═══')
mut_dir = tempfile.mkdtemp(prefix='shop64_mut_')
shutil.copy('adm/daemons/shop_d.c', os.path.join(mut_dir, 'shop_d.c'))
mut = readf(os.path.join(mut_dir, 'shop_d.c'))
# 突变：把一条有效丹药引用改回 #97 清单里的死路径
mut2 = mut.replace('"/clone/pill/juqi"', '"/obj/weapon/huangfeng_sword"')
if mut2 == mut:
    err('突变注入失败：未找到 /clone/pill/juqi 可替换')
else:
    bad2 = guard(mut2, '突变')
    if bad2:
        ok('一条改回死路径 → 守卫红（%d 条失败消息）' % len(bad2))
    else:
        err('突变失败：改回死路径后守卫仍绿')
shutil.rmtree(mut_dir, ignore_errors=True)

print('\n═══ 四、购买失败两类拆分契约（-6 数据异常 / -7 背包无法容纳） ═══')
fac = readf('cmds/usr/faction.c')
bad3 = split_guard(shop, fac)
if not bad3:
    ok('buy_item 存在 return -7；faction.c 存在 case -7；两条提示分别含「商品数据异常」/「背包无法容纳」且均明示「未扣除任何费用」')
else:
    for b in bad3:
        err(b)

print('\n═══ 五、突变实证（-6/-7 拆分守卫非恒真） ═══')
# 突变：把 -7 拆分改回旧写法（buy_item 的 return -7 与 faction.c 的 case -7 一律改回 -6）
m_shop = shop.replace('return -7', 'return -6')
m_fac = fac.replace('case -7:', 'case -6:')
if m_shop == shop or m_fac == fac:
    err('突变注入失败：未找到可改回的 -7')
else:
    bad4 = split_guard(m_shop, m_fac)
    if bad4:
        ok('-7 改回 -6 → 守卫红（%d 条失败消息）: %s' % (len(bad4), ' | '.join(bad4)))
    else:
        err('突变失败：-7 改回 -6 后拆分守卫仍绿（守卫恒真）')

print('\n═══ 六、#105 退款专用裸回补契约（refund_reputation） ═══')
rep = readf('adm/daemons/reputation_d.c')
bad5 = refund_guard(shop, rep)
if not bad5:
    ok('refund_reputation 四参签名（reason 强制）、不触每日上限/势力互斥、强制写审计日志；buy_item 回滚改用该接口且不再出现 add_reputation')
else:
    for b in bad5:
        err(b)

print('\n═══ 七、突变实证（回滚改回普通加声望接口应红） ═══')
m_shop2 = shop.replace('refund_reputation(player, faction, rep_cost * quantity,',
                       'add_reputation(player, faction, rep_cost * quantity)')
if m_shop2 == shop:
    err('突变注入失败：未找到 buy_item 回滚处的 refund_reputation 调用')
else:
    bad6 = refund_guard(m_shop2, rep)
    if bad6:
        ok('回滚改回 add_reputation → 守卫红（%d 条失败消息）' % len(bad6))
    else:
        err('突变失败：回滚改回 add_reputation 后退款契约守卫仍绿（守卫恒真）')

print('\n═══════════════════════════════════')
print('断言总数: %d, 失败: %d' % (oks, fails))
if fails:
    print('FAILED: 存在失败项')
    sys.exit(1)
print('RESULT: PASS 全部通过')
