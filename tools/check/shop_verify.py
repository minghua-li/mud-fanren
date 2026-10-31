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

if 'add_reputation(player, faction' in buy:
    ok('交付失败回滚声望扣减')
else:
    err('交付失败缺少回滚（add_reputation 回补）')

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

print('\n═══════════════════════════════════')
print('断言总数: %d, 失败: %d' % (oks, fails))
if fails:
    print('FAILED: 存在失败项')
    sys.exit(1)
print('RESULT: PASS 全部通过')
