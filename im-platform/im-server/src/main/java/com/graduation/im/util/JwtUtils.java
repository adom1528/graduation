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
}