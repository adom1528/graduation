package com.graduation.im.service;

import com.graduation.im.entity.ChatUser;

import java.util.List;

public interface UserService {
    // 户籍科的本职工作：找人！
    List<ChatUser> searchUsersByNickname(String keyword);
}
