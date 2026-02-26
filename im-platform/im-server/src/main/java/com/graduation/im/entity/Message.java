package com.graduation.im.entity;

import lombok.Data;

@Data
public class Message {
    // 消息发送者ID (服务器收到消息后会自动填入)
    private Long fromUserId;

    // 消息接收者ID (前端传过来)
    private Long toUserId;

    // 消息内容
    private String content;

    // 消息类型 (暂定：1=单聊, 2=群聊)
    private Integer type;
}