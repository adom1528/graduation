package com.graduation.im.handler;

import com.graduation.im.util.JwtUtils;
import com.graduation.im.util.UserChannelCtxMap;
import io.jsonwebtoken.Claims;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.SimpleChannelInboundHandler;
import io.netty.handler.codec.http.FullHttpRequest;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;
import org.springframework.web.util.UriComponentsBuilder;
import java.util.List;
import java.util.Map;
import org.springframework.data.redis.core.StringRedisTemplate;
import com.fasterxml.jackson.databind.ObjectMapper;
import io.netty.handler.codec.http.websocketx.TextWebSocketFrame;
import io.netty.channel.Channel;
import java.util.HashMap;

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
    private static final ObjectMapper objectMapper = new ObjectMapper(); // 引入 JSON 工具
    // 注入Redis模板
    @Autowired
    private StringRedisTemplate stringRedisTemplate;

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

            // 6. 在 Redis 里记录用户已上线， key（业务名：模块名：具体ID）
            stringRedisTemplate.opsForValue().set("im:online:" + userId, "1");
            log.info("用户：" + username + "ID=" + userId + " 已上线，且存入redis。");

            // 7. 通知所有人我上线！
            Map<String, Object> statusMsg = new HashMap<>();
            statusMsg.put("type", 3); // 3 代表状态广播
            statusMsg.put("userId", userId);
            statusMsg.put("content", "online");
            String jsonOutput = objectMapper.writeValueAsString(statusMsg);
            TextWebSocketFrame frame = new TextWebSocketFrame(jsonOutput);

            // 遍历户籍室所有人群发
            for (Channel channel : UserChannelCtxMap.getAllChannels()) {
                if (channel.isActive()) {
                    channel.writeAndFlush(frame.retainedDuplicate());
                }
            }
            // 8. 重要！修改 URI，去掉 token 参数
            req.setUri("/im");

            // 9. 传递给下一个 Handler (即 WebSocket 握手处理器) 完成握手
            ctx.fireChannelRead(req.retain());

        } catch (Exception e) {
            log.error("Token 无效或已过期: {}", e.getMessage());
            ctx.close();
        }
    }
}