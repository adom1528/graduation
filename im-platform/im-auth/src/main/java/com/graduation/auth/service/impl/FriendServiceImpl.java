package com.graduation.auth.service.impl;

import com.graduation.auth.entity.FriendVO;
import com.graduation.auth.mapper.FriendMapper;
import com.graduation.auth.service.FriendService;
import jakarta.annotation.Resource;
import org.springframework.stereotype.Service;

import java.util.List;

@Service
public class FriendServiceImpl implements FriendService {

    @Resource // 或者 @Autowired
    private FriendMapper friendMapper;

    @Override
    public List<FriendVO> getFriendList(Long userId) {
        // 直接调用你刚才写好 SQL 的 Mapper
        return friendMapper.getFriendList(userId);
    }
}