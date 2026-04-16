package com.graduation.im.controller;

import com.graduation.im.util.JwtUtils;
import com.graduation.im.common.Result;
import com.graduation.im.entity.FriendVO;
import com.graduation.im.service.FriendService;
import jakarta.servlet.http.HttpServletRequest;
import lombok.extern.slf4j.Slf4j;
import org.springframework.web.bind.annotation.*;

import jakarta.annotation.Resource;
import java.util.List;

@Slf4j
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

    //发起添加好友申请
    @PostMapping("/request")
    public Result<?> sendFriendRequest(@RequestParam("targetUserId") Long targetUserId,
                                       @RequestParam(value = "reason", required = false) String reason,
                                       HttpServletRequest request) {

        // 从你的 Token 解析器或者拦截器中拿到当前登录用户的 ID
        // 这里假设你有个工具类或者在 request attribute 里存了 userId
        //Long currentUserId = (Long) request.getAttribute("userId");
        String token = request.getHeader("Authorization");
        //log.debug(token);
        // JwtUtils 直接拿 ID
        Long currentUserId = jwtUtils.getUserIdFromHeaderToken(token);
        if (currentUserId == null) {
            return Result.error("Token无效或已过期，请重新登录");
        }
        //log.debug(currentUserId.toString());

        try {
            friendService.sendFriendRequest(currentUserId, targetUserId, reason);
            return Result.success("好友申请已发送");
        } catch (RuntimeException e) {
            // 捕获我们 Service 层抛出的业务异常，返回给前端展示
            return Result.error(e.getMessage());
        }
    }
}