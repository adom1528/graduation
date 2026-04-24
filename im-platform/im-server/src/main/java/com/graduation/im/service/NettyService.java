package com.graduation.im.service;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.graduation.im.util.UserChannelCtxMap;
import io.netty.channel.Channel;
import io.netty.handler.codec.http.websocketx.TextWebSocketFrame;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

import java.util.HashMap;
import java.util.Map;

/**
 * Spring Boot 与 Netty 通信的专门桥梁服务
 */
@Slf4j
@Service
public class NettyService {

    private static final ObjectMapper objectMapper = new ObjectMapper();

    /**
     * 发送“好友申请已通过”的强制刷新通知 (Type 6)
     * @param targetUserId 对方的 ID (发起申请的人)
     */
    public void sendFriendAcceptNotification(Long targetUserId) {
        // 1. 去户籍室找这个人到底在不在线
        Channel channel = UserChannelCtxMap.getChannel(targetUserId);

        if (channel != null && channel.isActive()) {
            try {
                // 2. 组装 Type = 6 的系统指令
                Map<String, Object> msg = new HashMap<>();
                msg.put("type", 6);
                msg.put("content", "好友申请已通过，请刷新列表");

                String jsonOutput = objectMapper.writeValueAsString(msg);

                // 3. 顺着网线砸过去！
                channel.writeAndFlush(new TextWebSocketFrame(jsonOutput));
                log.info("🎯 已向用户 {} 推送好友列表刷新指令 (Type 6)", targetUserId);
            } catch (Exception e) {
                log.error("推送刷新指令失败", e);
            }
        } else {
            log.info("用户 {} 不在线，无需推送刷新指令", targetUserId);
        }
    }

    /**
     * 发送“收到新好友申请”的通知 (Type 7) - 预留给你后面做红点提醒用
     * @param targetUserId 接收申请的人
     */
    public void sendFriendRequestNotification(Long targetUserId) {
        Channel channel = UserChannelCtxMap.getChannel(targetUserId);

        if (channel != null && channel.isActive()) {
            try {
                Map<String, Object> msg = new HashMap<>();
                msg.put("type", 7);
                msg.put("content", "您有新的好友申请");

                String jsonOutput = objectMapper.writeValueAsString(msg);
                channel.writeAndFlush(new TextWebSocketFrame(jsonOutput));
                log.info("📢 已向用户 {} 推送新好友申请通知 (Type 7)", targetUserId);
            } catch (Exception e) {
                log.error("推送申请通知失败", e);
            }
        }
    }
}