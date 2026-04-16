package com.graduation.im.service.impl;

import com.baomidou.mybatisplus.extension.service.impl.ServiceImpl;
import com.graduation.im.entity.ChatUser;
import com.graduation.im.mapper.UserMapper;
import com.graduation.im.service.UserService;
import jakarta.annotation.Resource;
import org.springframework.stereotype.Service;

import java.util.List;

@Service
public class UserServiceImpl extends ServiceImpl<UserMapper, ChatUser> implements UserService {

    @Resource
    private UserMapper userMapper;

    @Override
    public List<ChatUser> searchUsersByNickname(String keyword) {
        if (keyword == null || keyword.trim().isEmpty()) {
            return java.util.Collections.emptyList();
        }
        return userMapper.searchUsersByNickname(keyword.trim());
    }
}