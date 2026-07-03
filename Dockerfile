FROM fluffos/fluffos:latest

WORKDIR /app

# 复制游戏文件（通过 .dockerignore 排除不必要的文件）
COPY . .

# 确保运行时目录存在（log/ 被 .dockerignore 排除，data/ 保留初始数据）
RUN mkdir -p /app/log

# 游戏端口：5555(GBK telnet)、6666(UTF-8 telnet)、8888(websocket)
EXPOSE 5555 6666 8888

# /fluffos/bin/driver 是镜像的 ENTRYPOINT，此处只需传入配置文件路径
CMD ["config.ini"]
