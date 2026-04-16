package com.graduation.im.entity; // 替换成你实际的包名

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;
import java.util.Date;

/**
 * 🌟 业务端专属用户视图 (隔离敏感信息，贯彻 DDD 思想)
 */
@Data
@TableName("im_user") // 依然映射到同一张表，但只取我们需要的数据
public class ChatUser {

    @TableId(type = IdType.ASSIGN_ID) // 雪花算法
    private Long id;

    private String username;

    private String nickname;

    private String avatar;

    private Date createTime;

    private Date updateTime;
}