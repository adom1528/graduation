package com.graduation.im.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.graduation.im.entity.FriendRequest;
import com.graduation.im.entity.FriendRequestVO;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Select;

import java.util.List;

@Mapper
public interface FriendRequestMapper extends BaseMapper<FriendRequest> {

    /**
     * 检查是否已经存在“待处理(status=0)”的好友申请
     * 防止用户疯狂点击“添加”按钮，给对方发一百条一样的申请
     */
    @Select("SELECT COUNT(*) FROM im_friend_request " +
            "WHERE from_user_id = #{fromUserId} AND to_user_id = #{toUserId} AND status = 0")
    int checkPendingRequest(@Param("fromUserId") Long fromUserId, @Param("toUserId") Long toUserId);

    /**
     * 联表查询当前用户的所有待处理好友申请
     * 注意：请将 im_chat_user 替换为你数据库中真实的用户表名
     */
    @Select("SELECT r.id AS requestId, r.from_user_id, r.reason, r.create_time, " +
            "u.username, u.nickname, u.avatar " +
            "FROM im_friend_request r " +
            "INNER JOIN im_user u ON r.from_user_id = u.id " +
            "WHERE r.to_user_id = #{currentUserId} AND r.status = 0 " +
            "ORDER BY r.create_time DESC")
    List<FriendRequestVO> getPendingRequests(@Param("currentUserId") Long currentUserId);
}