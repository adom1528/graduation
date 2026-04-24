package com.graduation.im.entity;
import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;
import java.util.Date;

/**
 * 好友申请流转表实体类
 */
@Data
@TableName("im_friend_request")
public class FriendRequest {

    // 如果你的主键习惯用雪花算法，这里可以改成 IdType.ASSIGN_ID
    @TableId(type = IdType.AUTO)
    private Long id;

    /**
     * 发起方用户ID
     */
    private Long fromUserId;

    /**
     * 接收方用户ID
     */
    private Long toUserId;

    /**
     * 验证信息(例如: "你好，我是zs")
     */
    private String reason;

    /**
     * 状态: 0-待处理, 1-已同意, 2-已拒绝
     */
    private Integer status;

    private Date createTime;

    private Date updateTime;
}