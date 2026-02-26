package com.graduation.im.handler;

import com.graduation.im.util.JwtUtils;
import com.graduation.im.util.UserChannelCtxMap;
import io.jsonwebtoken.Claims;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.SimpleChannelInboundHandler;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.websocketx.WebSocketServerProtocolHandler;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;
import org.springframework.web.util.UriComponentsBuilder;

import java.util.List;
import java.util.Map;

/**
 * 专门处理握手时的身份认证
 * 它的位置必须在 HttpServerCodec 之后，WebSocketServerProtocolHandler 之前
 */
// 加上这个核心注解，告诉 Netty 这个类可以被多个连接共享
@io.netty.channel.ChannelHandler.Sharable
@Slf4j
@Component
public class WebSocketTokenHandler extends SimpleChannelInboundHandler<FullHttpRequest> {

    @Autowired
    private JwtUtils jwtUtils;

    @Override
    protected void channelRead0(ChannelHandlerContext ctx, FullHttpRequest req) throws Exception {
        // 1. 获取请求路径
        String uri = req.uri();

        // 2. 只有请求 WebSocket 路径 (/im) 时才拦截，其他的(如心跳检测)放行
        if (!uri.startsWith("/im")) {
            ctx.fireChannelRead(req.retain()); // 传递给下一个 Handler
            return;
        }

        // 3. 解析 URL 参数，提取 token
        // 例如：/im?token=xxxxx
        Map<String, List<String>> queryParams = UriComponentsBuilder.fromUriString(uri)
                .build()
                .getQueryParams();

        List<String> tokens = queryParams.get("token");
        String token = (tokens != null && !tokens.isEmpty()) ? tokens.get(0) : null;

        if (token == null) {
            log.error("连接被拒绝：未携带Token");
            ctx.close(); // 直接关闭连接
            return;
        }

        // 4. 验证 Token
        try {
            Claims claims = jwtUtils.parseToken(token);
            // ID 是雪花算法生成的超长数字，它超出了 Integer 的范围，用Number更好
            Long userId = ((Number) claims.get("userId")).longValue();
            String username = (String) claims.get("username");

            log.info("鉴权通过，用户: {} (ID={})", username, userId);

            // 5. 绑定身份 (存入户籍室)
            UserChannelCtxMap.addChannel(userId, ctx.channel());

            // 6. 将 Netty 的 AttributeKey 设置好 (可选，方便后续 Handler 获取用户信息)
            // AttributeKey<Long> key = AttributeKey.valueOf("userId");
            // ctx.channel().attr(key).set(userId);

            // 7. 重要！修改 URI，去掉 token 参数
            // 因为 Netty 的 WebSocketServerProtocolHandler 对路径匹配很严格
            // 如果不做这步，昨天还需要设置 checkStartsWith=true，做了这步就可以恢复 strict 模式
            req.setUri("/im");

            // 8. 传递给下一个 Handler (即 WebSocket 握手处理器) 完成握手
            ctx.fireChannelRead(req.retain());

        } catch (Exception e) {
            log.error("Token 无效或已过期: {}", e.getMessage());
            ctx.close();
        }
    }
}