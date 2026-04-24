package com.graduation.im.handler;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.graduation.im.entity.Message;
import com.graduation.im.util.UserChannelCtxMap;
import io.netty.channel.Channel;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.SimpleChannelInboundHandler;
import io.netty.handler.codec.http.websocketx.TextWebSocketFrame;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;
import com.graduation.im.service.ChatMessageService;
import io.netty.handler.timeout.IdleState;
import io.netty.handler.timeout.IdleStateEvent;
import org.springframework.data.redis.core.StringRedisTemplate;

import java.util.HashMap;
import java.util.Map;

@Slf4j
@Component // 交给 Spring 管理
@io.netty.channel.ChannelHandler.Sharable // 允许被多个 Channel 共享
public class SimpleHandler extends SimpleChannelInboundHandler<TextWebSocketFrame> {

    // Jackson 的 JSON 工具
    private static final ObjectMapper objectMapper = new ObjectMapper();
    // 数据库服务注入
    @Autowired
    private ChatMessageService chatMessageService;

    @Autowired
    private StringRedisTemplate stringRedisTemplate;

    @Override
    protected void channelRead0(ChannelHandlerContext ctx, TextWebSocketFrame frame) throws Exception {
        String text = frame.text();
        // 如果是心跳包，回复一个 "pong" 就直接结束，不走数据库逻辑
        if ("ping".equals(text)) {
            // log.debug("收到心跳: ping, 回复: pong");
            ctx.channel().writeAndFlush(new TextWebSocketFrame("pong"));
            return;
        }

        log.info("收到原始消息: {}", text);

        // 1. 解析 JSON
        Message msg;
        try {
            msg = objectMapper.readValue(text, Message.class);
        } catch (Exception e) {
            //log.error("JSON格式错误");
            log.error("🔴 JSON解析失败！前端发来的原始文本是: [{}]", text, e);
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

        // 3. 路由转发
        Long toUserId = msg.getToUserId();
        Channel targetChannel = UserChannelCtxMap.getChannel(toUserId);
        // 不管对方在不在，这句聊天记录必须永久存入数据库
        int type = msg.getType();
        // 判断是文件时才传文件名
        if (type == 5) {
            chatMessageService.saveMessage(fromUserId, toUserId, type, msg.getContent(), msg.getFileName());
        } else {
            chatMessageService.saveMessage(fromUserId, toUserId, type, msg.getContent());
        }


        if (targetChannel != null && targetChannel.isActive()) {
            // A. 对方在线 -> 直接转发
            // 把对象转回 JSON 字符串发送
            String jsonOutput = objectMapper.writeValueAsString(msg);
            targetChannel.writeAndFlush(new TextWebSocketFrame(jsonOutput));
            log.info("消息已转发给用户: {}", toUserId);
        } else {
            // B. 对方不在线
            // 简单处理：给发送者回个信
            String errorMsg = "对方(ID=" + toUserId + ")不在线,但消息已存入数据库";
            ctx.channel().writeAndFlush(new TextWebSocketFrame(errorMsg));
            log.info("消息转发失败（不在线），但已存入数据库: {}", errorMsg);
        }
    }

    // 捕获 IdleStateHandler 抛出的超时事件
    @Override
    public void userEventTriggered(ChannelHandlerContext ctx, Object evt) throws Exception {
        if (evt instanceof IdleStateEvent) {
            IdleStateEvent event = (IdleStateEvent) evt;
            if (event.state() == IdleState.READER_IDLE) {
                // 超过 60 秒没收到前端的任何消息或心跳，判定为僵尸连接
                log.warn("🔴 雷达警报：发现僵尸连接，已强制踢下线! ChannelID: {}", ctx.channel().id());
                // 关闭通道！(通道关闭会自动触发 channelInactive，那里会把人从 UserChannelCtxMap 里删掉)
                ctx.close();
            }
        } else {
            super.userEventTriggered(ctx, evt);
        }
    }

    // 连接断开时，记得把人从户籍室踢出去并在Redis里记录
    @Override
    public void channelInactive(ChannelHandlerContext ctx) throws Exception {
        // 1. 从户籍室反查出这个通道对应的 UserId
        Long userId = UserChannelCtxMap.getUserId(ctx.channel());

        if (userId != null) {
            // 把redis上的名字擦掉！
            stringRedisTemplate.delete("im:online:" + userId);
            log.info("用户下线，已清除 Redis 在线状态: ID={}", userId);

            // 2. 下线通知所有人
            try {
                Map<String, Object> statusMsg = new HashMap<>();
                statusMsg.put("type", 3);
                statusMsg.put("userId", userId);
                statusMsg.put("content", "offline");
                String jsonOutput = objectMapper.writeValueAsString(statusMsg);
                TextWebSocketFrame frame = new TextWebSocketFrame(jsonOutput);

                for (Channel channel : UserChannelCtxMap.getAllChannels()) {
                    // 发给除自己以外的其他活着的通道
                    if (channel != ctx.channel() && channel.isActive()) {
                        channel.writeAndFlush(frame.retainedDuplicate());
                    }
                }
            } catch (Exception e) {
                log.error("发送下线广播失败", e);
            }
        }

        // 3. 从本地户籍室移除
        UserChannelCtxMap.removeChannel(ctx.channel());
        log.info("连接断开，已移除映射: {}", ctx.channel().id());

        super.channelInactive(ctx);
    }
}