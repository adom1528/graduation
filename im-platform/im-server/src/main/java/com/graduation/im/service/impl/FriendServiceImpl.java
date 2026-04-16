package com.graduation.im.service.impl;

import com.baomidou.mybatisplus.core.toolkit.IdWorker;
import com.baomidou.mybatisplus.extension.service.impl.ServiceImpl;
import com.graduation.im.entity.ChatUser;
import com.graduation.im.entity.FriendRequest;
import com.graduation.im.entity.FriendVO;
import com.graduation.im.mapper.FriendMapper;
import com.graduation.im.mapper.FriendRequestMapper;
import com.graduation.im.mapper.UserMapper;
import com.graduation.im.service.FriendService;
import jakarta.annotation.Resource;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.redis.core.StringRedisTemplate;
import java.util.List;


@Service
public class FriendServiceImpl extends ServiceImpl<FriendMapper, FriendVO> implements FriendService {

    @Resource // 或者 @Autowired
    private FriendMapper friendMapper;

    @Resource
    private FriendRequestMapper friendRequestMapper;
    @Autowired
    private StringRedisTemplate stringRedisTemplate;

    @Override
    public List<FriendVO> getFriendList(Long userId) {
        // 直接调用SQL 的 Mapper
        List<FriendVO> friendList = friendMapper.getFriendList((userId));

        // 去 Netty 户籍室查询好友在线情况
        if (friendList != null && !friendList.isEmpty()) {
            for (FriendVO friend : friendList) {
                // 拼接Netty设定的key
                String redisKey = "im:online:" + friend.getId();

                // 在redis查找key是否存在
                Boolean isOnline = stringRedisTemplate.hasKey((redisKey));

                friend.setIsOnline(isOnline != null && isOnline);
            }
        }
        // 后续优化，不用for，把所有好友的key收集成一个List，再用redis的multiGet拉取
        return friendList;
    }

    @Transactional(rollbackFor = Exception.class)
    public void sendFriendRequest(Long currentUserId, Long targetUserId, String reason) {

        // 不能加自己
        if (currentUserId.equals(targetUserId)) {
            throw new RuntimeException("不能添加自己为好友");
        }

        // 检查是否已经是好友了
        if (friendMapper.checkIsFriend(currentUserId, targetUserId) > 0) {
            throw new RuntimeException("对方已经是你的好友了，无需重复添加");
        }

        // 防线 3：检查是否已经发过申请，且对方还没处理 (防刷单)
        if (friendRequestMapper.checkPendingRequest(currentUserId, targetUserId) > 0) {
            throw new RuntimeException("您已发送过申请，请耐心等待对方处理");
        }

        // 极其隐蔽的交互逻辑 —— 检查对方是不是也恰好给我发了申请？
        // 如果对方也正在申请加我，那这波叫“双向奔赴”，直接通过！(这里作为进阶，目前先跑通单向)
        // int reverseRequest = friendRequestMapper.checkPendingRequest(targetUserId, currentUserId);
        // if (reverseRequest > 0) { ... 直接加好友 ... }

        // 正式添加好友
        FriendRequest request = new FriendRequest();
        request.setFromUserId(currentUserId);
        request.setToUserId(targetUserId);
        // 如果前端没传理由，给个默认的礼貌用语
        request.setReason(reason != null && !reason.trim().isEmpty() ? reason : "你好，我想添加你为好友");
        request.setStatus(0); // 0 代表：待处理

        // 存入数据库！
        friendRequestMapper.insert(request);

        // 🌟 K导师的伏笔：在这里，我们未来需要调用 Netty 给 targetUserId 发送一条实时通知！
        // nettyService.sendFriendRequestNotification(targetUserId);

    }
}