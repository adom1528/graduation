package com.graduation.im.entity;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import lombok.Data;

@Data
@JsonIgnoreProperties(ignoreUnknown = true)
public class Message {
    // 消息发送者ID (服务器收到消息后会自动填入)
    private Long fromUserId;

    // 消息接收者ID (前端传过来)
    private Long toUserId;

    // 消息内容
    private String content;

    // 接收文件/图片的字段
    private String fileName;

    // 消息类型 (暂定：1=单聊, 2=群聊, 3=在线状态信息变更, 4=图片，5=文件, 6=好友列表刷新通知)
    private Integer type;
}