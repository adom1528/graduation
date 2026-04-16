package com.graduation.im.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.graduation.im.entity.ChatUser; // 引入新实体
import org.apache.ibatis.annotations.Mapper;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Select;

import java.util.List;

@Mapper
public interface UserMapper extends BaseMapper<ChatUser> {

    @Select("SELECT id, username, nickname, avatar, create_time " +
            "FROM im_user " +
            "WHERE nickname LIKE CONCAT('%', #{keyword}, '%') " +
            "LIMIT 20")
    List<ChatUser> searchUsersByNickname(@Param("keyword") String keyword);

    @Select("SELECT id, username, nickname, avatar, create_time " +
            "FROM im_user " +
            "WHERE id = #{userId}")
    ChatUser getUserBasicInfoById(@Param("userId") Long userId);
}