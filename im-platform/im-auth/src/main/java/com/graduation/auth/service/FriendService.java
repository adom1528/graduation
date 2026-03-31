package com.graduation.auth.service;

import com.graduation.auth.entity.FriendVO;

import java.util.List;

public interface FriendService {
    List<FriendVO> getFriendList(Long userId);
}
