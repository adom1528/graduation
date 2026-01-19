package com.graduation.im.handler;

import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.SimpleChannelInboundHandler;
import io.netty.handler.codec.http.websocketx.TextWebSocketFrame;
import lombok.extern.slf4j.Slf4j;

import java.time.LocalDateTime;

/**
 * 这是一个简单的处理器，专门处理文本消息 (TextWebSocketFrame)
 */
@Slf4j
public class SimpleHandler extends SimpleChannelInboundHandler<TextWebSocketFrame> {

    // 当客户端连接上来时触发
    @Override
    public void channelActive(ChannelHandlerContext ctx) throws Exception {
        log.info("客户端连接成功: {}", ctx.channel().id());
    }

    // 当客户端断开连接时触发
    @Override
    public void channelInactive(ChannelHandlerContext ctx) throws Exception {
        log.info("客户端断开连接: {}", ctx.channel().id());
    }

    // 当收到消息时触发
    @Override
    protected void channelRead0(ChannelHandlerContext ctx, TextWebSocketFrame msg) throws Exception {
        String text = msg.text();
        log.info("收到消息: {}", text);

        // 业务逻辑：原本应该转发给别人，现在我们先做一个“回声”测试
        // 回复：[服务器时间] 你说的是: ...
        String response = "[" + LocalDateTime.now() + "] Server received: " + text;

        // 注意：Netty 中写数据必须封装成 Frame，不能直接写 String
        ctx.channel().writeAndFlush(new TextWebSocketFrame(response));
    }
}