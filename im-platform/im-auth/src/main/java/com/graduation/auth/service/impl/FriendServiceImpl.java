package com.graduation.auth.service.impl;

import com.baomidou.mybatisplus.core.toolkit.IdWorker;
import com.baomidou.mybatisplus.extension.service.impl.ServiceImpl;
import com.graduation.auth.entity.FriendVO;
import com.graduation.auth.mapper.FriendMapper;
import com.graduation.auth.mapper.UserMapper;
import com.graduation.auth.service.FriendService;
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
    private UserMapper userMapper;

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
    @Override
    public FriendVO searchUser(String username) {
        return userMapper.searchByUsername(username);
    }

    // 🌟 K导师的终极防御阵型：添加好友逻辑
    @Transactional(rollbackFor = Exception.class)
    @Override
    public void addFriend(Long currentUserId, Long targetUserId) {
        // 1.不能自己加自己
        if (currentUserId.equals(targetUserId)) {
            throw new RuntimeException("不能添加自己为好友");
        }

        // 2. 检查是否已经是好友
        if (friendMapper.checkIsFriend(currentUserId, targetUserId) > 0) {
            throw new RuntimeException("对方已经是你的好友了");
        }

        // 3. 生成两个雪花 ID
        Long id1 = IdWorker.getId();
        Long id2 = IdWorker.getId();

        // 4. 插入双向奔赴的两条记录
        friendMapper.insertFriend(id1, currentUserId, targetUserId); // 我 -> 对方
        friendMapper.insertFriend(id2, targetUserId, currentUserId); // 对方 -> 我
    }
}