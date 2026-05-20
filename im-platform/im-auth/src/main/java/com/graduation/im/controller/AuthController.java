package com.graduation.im.controller;

import com.graduation.im.common.Result;
import com.graduation.im.dto.res.LoginVO;
import com.graduation.im.service.UserService;
import com.graduation.im.dto.req.*;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/auth")
public class AuthController {

    @Autowired
    private UserService userService;

    // POST http://localhost:9001/auth/register
    @PostMapping("/register")
    public Result<String> register(@RequestBody RegisterRequest registerRequest) {
        try {
            // 直接将整个 DTO 丢给 Service 处理，让表现层极其轻量
            userService.register(registerRequest);
            return Result.success("注册成功");
        } catch (Exception e) {
            return Result.error(e.getMessage());
        }
    }

    // POST http://localhost:9001/auth/login
    @PostMapping("/login")
    public Result<LoginVO> login(@RequestBody LoginRequest loginRequest) { // 🌟 String 改为 LoginVO
        try {
            String username = loginRequest.getUsername();
            String password = loginRequest.getPassword();
            LoginVO loginVo = userService.login(username, password);
            return Result.success(loginVo); // 🌟 返回打包好的VO对象
        } catch (Exception e) {
            return Result.error(e.getMessage());
        }
    }

}