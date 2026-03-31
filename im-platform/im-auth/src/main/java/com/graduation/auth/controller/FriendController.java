package com.graduation.auth.controller;

import com.graduation.auth.common.JwtUtils;
import com.graduation.auth.common.Result;
import com.graduation.auth.entity.FriendVO;
import com.graduation.auth.service.FriendService;
import jakarta.servlet.http.HttpServletRequest;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import jakarta.annotation.Resource;
import java.util.List;

@RestController
@RequestMapping("/friend")
public class FriendController {

    @Resource
    private FriendService friendService;

    // 你的 JwtUtils 加了 @Component，直接注入即可！
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
}