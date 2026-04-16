package com.graduation.im.controller;

import com.graduation.im.common.Result;
import com.graduation.im.entity.ChatUser;
import com.graduation.im.service.UserService;
import jakarta.annotation.*;
import org.springframework.web.bind.annotation.*;

import java.util.List;


@RestController
@RequestMapping("/user") // 规范的 RESTful 路径：查用户就应该走 /user
public class UserController {

    @Resource
    private UserService userService;

    @GetMapping("/search")
    public Result<List<ChatUser>> searchUser(@RequestParam("nickname") String keyword) {
        // 让户籍科去查！
        return Result.success(userService.searchUsersByNickname(keyword));
    }
}
