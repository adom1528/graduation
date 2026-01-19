package com.graduation.im.init;

import com.graduation.im.handler.SimpleHandler;
import io.netty.channel.ChannelInitializer;
import io.netty.channel.ChannelPipeline;
import io.netty.channel.socket.SocketChannel;
import io.netty.handler.codec.http.HttpObjectAggregator;
import io.netty.handler.codec.http.HttpServerCodec;
import io.netty.handler.codec.http.websocketx.WebSocketServerProtocolHandler;
import io.netty.handler.stream.ChunkedWriteHandler;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

@Component
public class WebSocketChannelInitializer extends ChannelInitializer<SocketChannel> {

    // 从配置文件读取 WebSocket 路径 (/im)
    @Value("${im.server.websocket-path}")
    private String webSocketPath;

    @Override
    protected void initChannel(SocketChannel ch) throws Exception {
        ChannelPipeline pipeline = ch.pipeline();

        // 1. HTTP 编解码器 (WebSocket 握手也是基于 HTTP 的)
        pipeline.addLast(new HttpServerCodec());
        // 2. 以块方式写入 (支持大数据流)
        pipeline.addLast(new ChunkedWriteHandler());
        // 3. HTTP 消息聚合器 (把分块的 HTTP 数据合并成完整的 FullHttpRequest)
        pipeline.addLast(new HttpObjectAggregator(65536));

        // 4. WebSocket 协议处理器 (核心！)
        // 它负责处理握手(Handshake)、Ping/Pong、Close 等繁琐细节
        pipeline.addLast(new WebSocketServerProtocolHandler(webSocketPath));

        // 5. 自定义业务处理器 (刚才写的那个工人)
        pipeline.addLast(new SimpleHandler());
    }
}