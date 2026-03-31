package com.graduation.auth.controller;

import com.graduation.auth.common.JwtUtils;
import com.graduation.auth.common.Result;
import com.graduation.auth.entity.FriendVO;
import com.graduation.auth.service.FriendService;
import jakarta.servlet.http.HttpServletRequest;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import jakarta.annotation.Resource;
import java.util.List;

@RestController
@RequestMapping("/friend")
public class FriendController {

    @Resource
    private FriendService friendService;

    @Resource
    private JwtUtils jwtUtils;

    @GetMapping("/list")
    public Result<List<FriendVO>> getFriendList(HttpServletRequest request) {
        // 1. 迎宾：拿到请求头里的 Token 字符串
        String token = request.getHeader("Authorization");

        // 2. 验明正身：呼叫拆弹专家 JwtUtils 直接拿 ID
        Long currentUserId = jwtUtils.getUserIdFromHeaderToken(token);
        if (currentUserId == null) {
            return Result.error("Token无效或已过期，请重新登录"); // 完美复用你写的 Result.error()
        }

        // 3. 核心业务：交给 Service 去数据库捞人
        List<FriendVO> list = friendService.getFriendList(currentUserId);

        // 4. 送客：完美打包返回
        return Result.success(list); // 完美复用你写的 Result.success()
    }

    @GetMapping("/search")
    public Result<FriendVO> searchUser(String username) {
        if (username == null || username.trim().isEmpty()) {
            return Result.error("搜索账号不能为空");
        }
        FriendVO user = friendService.searchUser(username);
        if (user == null) {
            return Result.error("用户不存在");
        }
        return Result.success(user);
    }

    @PostMapping("/add")
    public Result<String> addFriend(HttpServletRequest request, Long targetUserId) {
        // 1. 验明正身
        String token = request.getHeader("Authorization");
        Long currentUserId = jwtUtils.getUserIdFromHeaderToken(token);
        if (currentUserId == null) {
            return Result.error("Token无效");
        }
        if (targetUserId == null) {
            return Result.error("目标用户ID不能为空");
        }

        // 2. 呼叫 Service 执行添加
        try {
            friendService.addFriend(currentUserId, targetUserId);
            return Result.success("添加成功");
        } catch (Exception e) {
            return Result.error(e.getMessage()); // 捕获 Service 抛出的异常并返回给前端
        }
    }
}