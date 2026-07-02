FROM fluffos/fluffos:latest
WORKDIR /app
COPY . .
EXPOSE 6666
CMD ["sleep", "infinity"]
