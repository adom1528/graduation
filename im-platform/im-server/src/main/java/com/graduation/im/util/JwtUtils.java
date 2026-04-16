package com.graduation.im.util;

import io.jsonwebtoken.Claims;
import io.jsonwebtoken.Jwts;
import io.jsonwebtoken.SignatureAlgorithm;
import io.jsonwebtoken.security.Keys;
import org.springframework.stereotype.Component;

import java.security.Key;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

@Component
public class JwtUtils {

    // 1. 定义秘钥 (实际开发应该写在配置文件里)
    // 长度必须足够长，否则会报错。这里随便写一串复杂的
    private static final String SECRET = "MyGraduationProjectSecretKeyForImSystem2026";
    private static final Key KEY = Keys.hmacShaKeyFor(SECRET.getBytes());

    // 2. 定义过期时间 (比如 24 小时)
    private static final long EXPIRATION = 24 * 60 * 60 * 1000L;

    /**
     * 生成 Token
     * @param userId 用户ID
     * @param username 用户名
     * @return 加密后的 Token 字符串
     */
    public String createToken(Long userId, String username) {
        Map<String, Object> claims = new HashMap<>();
        claims.put("userId", userId);
        claims.put("username", username);

        return Jwts.builder()
                .setClaims(claims) // 放入自定义信息
                .setSubject(username) // 设置主题
                .setIssuedAt(new Date()) // 签发时间
                .setExpiration(new Date(System.currentTimeMillis() + EXPIRATION)) // 过期时间
                .signWith(KEY, SignatureAlgorithm.HS256) // 签名算法
                .compact();
    }

    /**
     * 解析 Token (后面网关会用到，先写着)
     */
    public Claims parseToken(String token) {
        return Jwts.parserBuilder()
                .setSigningKey(KEY)
                .build()
                .parseClaimsJws(token)
                .getBody();
    }

    /**
     * 从请求头中的 Token 提取雪花 userId
     * 自动处理 Bearer 前缀并防止 Long 类型强转异常
     */
    public Long getUserIdFromHeaderToken(String token) {
        if (token == null || token.trim().isEmpty()) {
            return null;
        }
        // 自动剥离 "Bearer " 前缀
        if (token.startsWith("Bearer ")) {
            token = token.substring(7);
        }
        try {
            // 复用你写好的 parseToken 方法
            Claims claims = parseToken(token);
            // 安全提取 userId，完美避开 ClassCastException
            return Long.valueOf(claims.get("userId").toString());
        } catch (Exception e) {
            // Token 过期或被篡改时，解析会抛异常，这里可以简单捕获返回 null
            return null;
        }
    }
}