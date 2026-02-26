package com.graduation.im.handler;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.graduation.im.entity.Message;
import com.graduation.im.util.UserChannelCtxMap;
import io.netty.channel.Channel;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.SimpleChannelInboundHandler;
import io.netty.handler.codec.http.websocketx.TextWebSocketFrame;
import lombok.extern.slf4j.Slf4j;

@Slf4j
public class SimpleHandler extends SimpleChannelInboundHandler<TextWebSocketFrame> {

    // Jackson 的 JSON 工具
    private static final ObjectMapper objectMapper = new ObjectMapper();

    @Override
    protected void channelRead0(ChannelHandlerContext ctx, TextWebSocketFrame frame) throws Exception {
        String text = frame.text();
        log.info("收到原始消息: {}", text);

        // 1. 解析 JSON
        Message msg;
        try {
            msg = objectMapper.readValue(text, Message.class);
        } catch (Exception e) {
            log.error("JSON格式错误");
            return;
        }

        // 2. 填充发送者信息 (从户籍室反查，或者从 AttributeKey 拿)
        // 这里演示简单的反查：谁发的这条消息？
        Long fromUserId = UserChannelCtxMap.getUserId(ctx.channel());
        if (fromUserId == null) {
            log.warn("发送者未登录，忽略");
            ctx.close();
            return;
        }
        msg.setFromUserId(fromUserId);

        // 3. 路由转发 (单聊核心逻辑)
        Long toUserId = msg.getToUserId();
        Channel targetChannel = UserChannelCtxMap.getChannel(toUserId);

        if (targetChannel != null && targetChannel.isActive()) {
            // A. 对方在线 -> 直接转发
            // 把对象转回 JSON 字符串发送
            String jsonOutput = objectMapper.writeValueAsString(msg);
            targetChannel.writeAndFlush(new TextWebSocketFrame(jsonOutput));
            log.info("消息已转发给用户: {}", toUserId);
        } else {
            // B. 对方不在线
            // 简单处理：给发送者回个信
            String errorMsg = "对方(ID=" + toUserId + ")不在线";
            ctx.channel().writeAndFlush(new TextWebSocketFrame(errorMsg));
            log.info("消息转发失败: {}", errorMsg);
        }
    }

    // 连接断开时，记得把人从户籍室踢出去
    @Override
    public void channelInactive(ChannelHandlerContext ctx) throws Exception {
        UserChannelCtxMap.removeChannel(ctx.channel());
        log.info("连接断开，已移除映射: {}", ctx.channel().id());
        super.channelInactive(ctx);
    }
}