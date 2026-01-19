package com.graduation.auth.controller;

import com.graduation.auth.common.Result;
import com.graduation.auth.service.UserService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/auth")
public class AuthController {

    @Autowired
    private UserService userService;

    // POST http://localhost:9001/auth/register
    @PostMapping("/register")
    public Result<String> register(@RequestParam String username, @RequestParam String password) {
        try {
            userService.register(username, password);
            return Result.success("注册成功");
        } catch (Exception e) {
            return Result.error(e.getMessage());
        }
    }

    // POST http://localhost:9001/auth/login
    @PostMapping("/login")
    public Result<String> login(@RequestParam String username, @RequestParam String password) {
        try {
            String token = userService.login(username, password);
            // 将 Token 返回给前端
            return Result.success(token);
        } catch (Exception e) {
            return Result.error(e.getMessage());
        }
    }
}