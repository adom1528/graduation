package com.graduation.im.controller;

import com.graduation.im.common.Result; // 记得导入你刚复制过来的 Result 类
import com.graduation.im.entity.ChatMessage;
import com.graduation.im.service.ChatMessageService;
import com.graduation.im.util.JwtUtils;
import io.jsonwebtoken.Claims;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import jakarta.servlet.http.HttpServletRequest;
import java.util.List;

@RestController
@RequestMapping("/chat")
public class ChatController {

    @Autowired
    private ChatMessageService chatMessageService;

    @Autowired
    private JwtUtils jwtUtils;

    @GetMapping("/history")
    public Result<List<ChatMessage>> getHistory(HttpServletRequest request, @RequestParam("friendId") Long friendId) {
        try {
            // 1. 从请求头中提取并解析 Token
            String token = request.getHeader("Authorization");
            if (token != null && token.startsWith("Bearer ")) {
                token = token.substring(7); // 剥离 "Bearer " 前缀
            } else {
                return Result.error("未携带有效 Token");
            }

            Claims claims = jwtUtils.parseToken(token);
            Long currentUserId = ((Number) claims.get("userId")).longValue();

            // 2. 呼叫 Service 查库！
            List<ChatMessage> historyList = chatMessageService.getHistory(currentUserId, friendId);

            // 3. 凯旋而归
            return Result.success(historyList);

        } catch (Exception e) {
            return Result.error("获取聊天记录失败：" + e.getMessage());
        }
    }
}