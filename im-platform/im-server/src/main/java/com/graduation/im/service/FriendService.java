package com.graduation.im.service;

import com.graduation.im.entity.FriendRequestVO;
import com.graduation.im.entity.FriendVO;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;

public interface FriendService {
    List<FriendVO> getFriendList(Long userId);
    @Transactional(rollbackFor = Exception.class)
    public void sendFriendRequest(Long currentUserId, Long targetUserId, String reason);

    void acceptFriendRequest(Long currentUserId, Long requestId);

    void rejectFriendRequest(Long currentUserId, Long requestId);

    List<FriendRequestVO> getPendingRequests(Long currentUserId);
    /**
     * 强事务级联删除好友
     * @param currentUserId 当前操作用户ID（从Token解析）
     * @param friendId      要删除的好友雪花ID
     */
    void deleteFriendCascade(Long currentUserId, Long friendId);
}
