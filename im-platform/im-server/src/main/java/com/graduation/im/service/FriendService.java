package com.graduation.im.service;

import com.graduation.im.entity.FriendVO;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;

public interface FriendService {
    List<FriendVO> getFriendList(Long userId);
    @Transactional(rollbackFor = Exception.class)
    public void sendFriendRequest(Long currentUserId, Long targetUserId, String reason);
}
