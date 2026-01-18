package com.graduation.auth.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;
@Data // Lombok: 自动生成 Gettter/Setter/ToString
@TableName("im_user") // 对应数据库：im_user（MyBatis Plus）
public class User {

    /**
     * 主键 ID
     * ASSIGN_ID 代表使用 MyBatis Plus 自带的雪花算法生成 ID
     * 对应数据库的 BIGINT
     */
    @TableId(type = IdType.ASSIGN_ID)
    private Long id;

    /**
     * 用户名
     */
    private String username;

    /**
     * 密码
     */
    private String password;

    /**
     * 昵称
     */
    private String nickname;

    /**
     * 头像
     */
    private String avatar;

    /**
     * 性别
     */
    private Integer sex;

    /**
     * 创建时间
     */
    private LocalDateTime createTime;

    /**
     * 更新时间
     */
    private LocalDateTime updateTime;
}
