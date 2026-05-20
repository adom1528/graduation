package com.graduation.im.service;

import com.baomidou.mybatisplus.extension.service.IService;
import com.graduation.im.dto.res.LoginVO;
import com.graduation.im.entity.User;
import com.graduation.im.dto.req.RegisterRequest;

public interface UserService extends IService<User> {
    // 参数直接替换为 RegisterRequest
    void register(RegisterRequest request);

    // 返回值从 String 变更为 LoginVO
    LoginVO login(String username, String password);
}