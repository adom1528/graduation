package com.graduation.im.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.graduation.im.entity.FriendRequest;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Select;

@Mapper
public interface FriendRequestMapper extends BaseMapper<FriendRequest> {

    /**
     * 检查是否已经存在“待处理(status=0)”的好友申请
     * 防止用户疯狂点击“添加”按钮，给对方发一百条一样的申请
     */
    @Select("SELECT COUNT(*) FROM im_friend_request " +
            "WHERE from_user_id = #{fromUserId} AND to_user_id = #{toUserId} AND status = 0")
    int checkPendingRequest(@Param("fromUserId") Long fromUserId, @Param("toUserId") Long toUserId);
}