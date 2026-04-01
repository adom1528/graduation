package com.graduation.im.entity;
import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;
import java.time.LocalDateTime;

@Data
@TableName("im_chat_message")
public class ChatMessage {
    @TableId(type = IdType.ASSIGN_ID)
    private Long id;
    private Long fromUserId;
    private Long toUserId;
    private String content;
    private Integer type;
    private LocalDateTime createTime;
}