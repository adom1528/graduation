package com.graduation.im.server;

import com.graduation.im.init.WebSocketChannelInitializer;
import io.netty.bootstrap.ServerBootstrap;
import io.netty.channel.ChannelFuture;
import io.netty.channel.EventLoopGroup;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.nio.NioServerSocketChannel;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.CommandLineRunner;
import org.springframework.stereotype.Component;

@Slf4j
@Component
public class NettyWebSocketServer implements CommandLineRunner {

    @Value("${im.server.port}")
    private int port; // 9003

    @Autowired
    private WebSocketChannelInitializer webSocketChannelInitializer;

    // Boss 线程组：专门负责处理连接请求 (像公司前台)
    private final EventLoopGroup bossGroup = new NioEventLoopGroup(1);
    // Worker 线程组：专门负责处理 IO 读写 (像干活的员工)
    private final EventLoopGroup workerGroup = new NioEventLoopGroup();

    @Override
    public void run(String... args) throws Exception {
        log.info("正在启动 Netty WebSocket 服务器...");
        new Thread(() -> {
            try {
                ServerBootstrap serverBootstrap = new ServerBootstrap();
                serverBootstrap.group(bossGroup, workerGroup)
                        .channel(NioServerSocketChannel.class)
                        .childHandler(webSocketChannelInitializer);

                // 绑定端口，同步等待成功
                ChannelFuture channelFuture = serverBootstrap.bind(port).sync();
                log.info("Netty WebSocket 服务器启动成功，端口: {}", port);

                // 等待服务端监听端口关闭
                channelFuture.channel().closeFuture().sync();
            } catch (InterruptedException e) {
                log.error("Netty 启动报错", e);
            } finally {
                bossGroup.shutdownGracefully();
                workerGroup.shutdownGracefully();
            }
        }).start(); // 必须另起线程，否则会阻塞 Spring Boot 主线程
    }
}