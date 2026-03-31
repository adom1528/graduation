package com.graduation.auth.service;

import com.graduation.auth.entity.FriendVO;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;

public interface FriendService {
    List<FriendVO> getFriendList(Long userId); // 获取好友列表

    FriendVO searchUser(String username); // 找寻user
    @Transactional(rollbackFor = Exception.class) // 开启事务！保证双向记录要么都成功，要么都失败！
    void addFriend(Long currentUserId, Long targetUserId);
}
