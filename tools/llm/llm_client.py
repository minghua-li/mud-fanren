from __future__ import annotations

"""LLM 配置与客户端（OpenAI 兼容 Chat Completions API）。

红线约定：LLM API 地址/密钥/模型名由玩家在自己主机本地配置——
只从环境变量或本地配置文件读取，绝不硬编码进代码库/提交/ticket。

配置优先级：
1. 命令行参数（--llm-base / --llm-key / --model）——进程级，临时
2. 本地配置文件 ~/.config/fanren-mud/llm.json —— 玩家手写，git 外
3. 环境变量 LLM_API_BASE / LLM_API_KEY / LLM_MODEL

本地配置文件格式（玩家自己创建，路径在 ~/.config/fanren-mud/llm.json）：
{
  "api_base": "https://api.openai.com/v1",
  "api_key": "sk-xxxx",
  "model": "gpt-4o-mini"
}
也可省略字段，缺省回退到对应环境变量。
"""

import json
import os
import urllib.request
import urllib.error
from pathlib import Path

DEFAULT_API_BASE = "https://api.openai.com/v1"
DEFAULT_MODEL = "gpt-4o-mini"
REQUEST_TIMEOUT = 60  # 秒；LLM 首字延迟普遍 1-30s，留足余量


def config_path() -> Path:
    """玩家本地配置文件路径（每次动态计算，尊重当前 HOME）。"""
    return Path.home() / ".config" / "fanren-mud" / "llm.json"


def _load_local_config() -> dict:
    """读取玩家本地配置文件（不存在或损坏时返回空 dict，不报错）。"""
    path = config_path()
    if not path.exists():
        return {}
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
        if isinstance(data, dict):
            return data
        return {}
    except (OSError, json.JSONDecodeError):
        return {}


class LLMConfig:
    """从环境变量/本地配置文件解析出的运行时配置（进程内，不落盘）。"""

    def __init__(self, api_base: str | None = None, api_key: str | None = None,
                 model: str | None = None):
        local = _load_local_config()
        self.api_base = (
            api_base
            or os.environ.get("LLM_API_BASE")
            or local.get("api_base")
            or DEFAULT_API_BASE
        ).rstrip("/")
        self.api_key = (
            api_key
            or os.environ.get("LLM_API_KEY")
            or local.get("api_key")
            or ""
        )
        self.model = (
            model
            or os.environ.get("LLM_MODEL")
            or local.get("model")
            or DEFAULT_MODEL
        )

    @property
    def configured(self) -> bool:
        """是否具备发起真实请求的条件。

        有 api_key 即可；本地 API（如 ollama）不要求 key——只要显式配置了
        自定义 api_base（非默认云端地址）就视为可用。完全未配置 → False。
        """
        if self.api_key:
            return True
        return self.api_base != DEFAULT_API_BASE

    def describe(self) -> str:
        """展示配置来源（绝不含密钥本体）。"""
        key_src = "环境变量 LLM_API_KEY" if os.environ.get("LLM_API_KEY") else (
            f"本地配置文件 {config_path()}" if _load_local_config().get("api_key") else "未配置"
        )
        return (f"API 地址: {self.api_base}\n"
                f"模型: {self.model}\n"
                f"密钥来源: {key_src}")


class LLMError(Exception):
    """LLM 调用失败的统一异常（超时/网络/HTTP 错误/非法响应）。"""


def _extract_json(text: str) -> dict:
    """从模型回复文本中提取 JSON 对象。

    先尝试整体解析；失败则找最外层花括号包裹的子串再解析；
    仍失败抛 LLMError（调用方走降级路径，绝不用解析失败的半成品）。
    """
    text = text.strip()
    if text.startswith("```"):
        # 容忍模型输出 markdown 代码块包裹
        text = text.strip("`")
        if text.startswith("json"):
            text = text[4:]
        text = text.strip()
    try:
        obj = json.loads(text)
        if isinstance(obj, dict):
            return obj
        raise LLMError(f"模型返回的不是 JSON 对象: {text[:200]}")
    except json.JSONDecodeError:
        pass
    # 找最外层花括号
    start = text.find("{")
    if start == -1:
        raise LLMError(f"模型回复中未找到 JSON: {text[:200]}")
    depth = 0
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                try:
                    obj = json.loads(text[start:i + 1])
                    if isinstance(obj, dict):
                        return obj
                except json.JSONDecodeError:
                    pass
                break
    raise LLMError(f"模型回复 JSON 解析失败: {text[:200]}")


def chat_completion(cfg: LLMConfig, system: str, user: str) -> dict:
    """调用 OpenAI 兼容 Chat Completions API，返回解析后的 JSON 对象。

    仅使用标准库 urllib；若服务端不支持 response_format，自动降级重试一次。
    """
    url = f"{cfg.api_base}/chat/completions"
    payload = {
        "model": cfg.model,
        "messages": [
            {"role": "system", "content": system},
            {"role": "user", "content": user},
        ],
        "temperature": 0.1,
        "response_format": {"type": "json_object"},
    }
    headers = {
        "Content-Type": "application/json",
    }
    if cfg.api_key:
        headers["Authorization"] = f"Bearer {cfg.api_key}"

    body = json.dumps(payload).encode("utf-8")
    for attempt in (1, 2):
        try:
            req = urllib.request.Request(url, data=body, headers=headers, method="POST")
            with urllib.request.urlopen(req, timeout=REQUEST_TIMEOUT) as resp:
                raw = resp.read().decode("utf-8")
            data = json.loads(raw)
            content = data["choices"][0]["message"]["content"]
            return _extract_json(content)
        except urllib.error.HTTPError as e:
            if attempt == 1 and e.code == 400:
                # 兼容端点不支持 response_format：去掉后重试一次
                payload.pop("response_format", None)
                body = json.dumps(payload).encode("utf-8")
                continue
            raise LLMError(f"LLM HTTP {e.code}: {e.read().decode('utf-8', errors='replace')[:300]}") from e
        except urllib.error.URLError as e:
            raise LLMError(f"LLM 网络错误: {e.reason}") from e
        except (KeyError, IndexError, json.JSONDecodeError) as e:
            raise LLMError(f"LLM 响应格式异常: {raw[:300]}") from e
        except TimeoutError as e:
            raise LLMError("LLM 请求超时") from e
    raise LLMError("LLM 请求失败")
