package com.graduation.im.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

@Data // Lombok: 自动生成 Getter/Setter/ToString
@TableName("im_friend") // 对应数据库：im_user（MyBatis Plus）

// 加上 lombok 注解 @Data
public class FriendVO {
    @TableId(type = IdType.ASSIGN_ID)
    private Long id;         // 重点：这是对方的雪花 ID，前端发消息要用
    private String username; // 账号
    private String nickname; // 昵称
    private String avatar;   // 头像 URL
    private Boolean isOnline; // 在线状态
}