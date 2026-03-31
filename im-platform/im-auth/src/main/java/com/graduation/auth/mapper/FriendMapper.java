package com.graduation.auth.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.graduation.auth.entity.FriendVO;
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Select;

import java.util.List;

@Mapper
public interface FriendMapper extends BaseMapper<FriendVO> {
    @Select("SELECT u.id, u.username, u.nickname, u.avatar " +
            "FROM im_friend f INNER JOIN im_user u ON f.friend_id = u.id " +
            "WHERE f.user_id = #{userId} AND f.status = 1")
    List<FriendVO> getFriendList(@Param("userId") Long userId);
}
