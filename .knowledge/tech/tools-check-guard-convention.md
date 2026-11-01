---
id: tools-check-guard-convention
title: tools/check 静态守卫写法约定（字符串断言查剥注释骨架）
tags: [tools-check, guard, lpc, regression]
updated: 2026-10-31
---

# tools/check 静态守卫写法约定

> 本仓无编译步骤（LPC 解释执行），`tools/check/*.py` 即项目测试面。守卫绑定交付物原文、且必须自带突变实证（真实突变 → 守卫红），否则是恒真守卫。

## 字符串存在断言必须查「剥注释骨架」

- 「函数体原文里存在字面量 X」这类断言，在目标语句被**注释掉**（而非删除）时仍恒绿——注释形态里同样含字面量。#64 复审实测出该洞：`root_refine_d.c` 把 `prob += pill_bonus;` 改成 `// prob += pill_bonus;` 后 pill_verify 旧断言全绿。
- 约定：先用 `strip_lpc()`（剥 `//` 行注释 / `/* */` 块注释 / 字符串字面量 / @LONG heredoc，保留代码骨架）把原文降到骨架再做 `in` 检查。pill_verify 的「大境界概率把 pill_bonus 加进概率」断言已按此修复（commit bd8e0b24）：整行删除 → 红、改成行注释 → 红、还原 → 绿。
- 同族脚本（shop_verify / forge_verify / main_quest_verify 等）的字符串存在断言在审查与补强时应同法核查；新增守卫时直接按此写法。
- 突变实证口径（至少三组）：① 整行删除 → 应红 ② 改成行注释 → 应红 ③ 还原 → 转绿。
