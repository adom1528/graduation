package com.graduation.im.controller;

import com.graduation.im.common.Result;
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
            String username = registerRequest.getUsername();
            String password = registerRequest.getPassword();
            String nickname = registerRequest.getNickname();
            userService.register(username, nickname, password);
            return Result.success("注册成功");
        } catch (Exception e) {
            return Result.error(e.getMessage());
        }
    }

    // POST http://localhost:9001/auth/login
    @PostMapping("/login")
    public Result<String> login(@RequestBody LoginRequest loginRequest) {
        try {
            String username = loginRequest.getUsername();
            String password = loginRequest.getPassword();
            String token = userService.login(username, password);
            // 将 Token 返回给前端
            return Result.success(token);
        } catch (Exception e) {
            return Result.error(e.getMessage());
        }
    }

}