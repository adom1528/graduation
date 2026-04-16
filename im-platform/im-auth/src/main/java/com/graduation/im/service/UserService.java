package com.graduation.im.service;

import com.baomidou.mybatisplus.extension.service.IService;
import com.graduation.im.entity.User;

public interface UserService extends IService<User> {
    // 定义一个注册方法
    void register(String username, String nickname, String password);
    // 返回值是 String，因为要返回 Token
    String login(String username, String password);
}