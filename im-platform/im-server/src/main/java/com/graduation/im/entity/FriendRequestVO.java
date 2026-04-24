package com.graduation.im.entity;

import lombok.Data;
import java.util.Date;

/**
 * 待处理好友申请的展示对象 (View Object)
 */
@Data
public class FriendRequestVO {
    // 申请记录本身的 ID (前端调用同意/拒绝接口时需要这个)
    private Long requestId;

    // 发起申请的人的 ID
    private Long fromUserId;

    // 🌟 联表查询带出来的发起人信息
    private String username;  // 唯一账号
    private String nickname;  // 昵称
    private String avatar;    // 头像 (如果有)

    // 申请理由和时间
    private String reason;
    private Date createTime;
}