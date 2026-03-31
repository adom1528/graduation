package com.graduation.auth.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.graduation.auth.entity.FriendVO;
import org.apache.ibatis.annotations.*;

import java.util.List;

@Mapper
public interface FriendMapper extends BaseMapper<FriendVO> {
    @Select("SELECT u.id, u.username, u.nickname, u.avatar " +
            "FROM im_friend f INNER JOIN im_user u ON f.friend_id = u.id " +
            "WHERE f.user_id = #{userId} AND f.status = 1")
    List<FriendVO> getFriendList(@Param("userId") Long userId);

    // 校验是否已经是好友
    @Select("SELECT COUNT(1) FROM im_friend WHERE user_id = #{userId} AND friend_id = #{friendId}")
    int checkIsFriend(@Param("userId") Long userId, @Param("friendId") Long friendId);

    // 插入好友记录 (注意：这里没用 MyBatis-Plus 的 insert，写原生 SQL 更直观)
    @Insert("INSERT INTO im_friend (id, user_id, friend_id, status, create_time) " +
            "VALUES (#{id}, #{userId}, #{friendId}, 1, NOW())")
    int insertFriend(@Param("id") Long id, @Param("userId") Long userId, @Param("friendId") Long friendId);
}
