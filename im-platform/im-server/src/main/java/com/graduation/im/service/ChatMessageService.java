package com.graduation.im.service;
import com.baomidou.mybatisplus.extension.service.IService;
import com.graduation.im.entity.ChatMessage;

import java.util.List;

public interface ChatMessageService extends IService<ChatMessage> {
    void saveMessage(Long fromId, Long toId, int type, String content, String fileName); // 文件需要文件名，其他不必
    void saveMessage(Long fromId, Long toId, int type, String content);
    List<ChatMessage> getHistory(Long userId, Long friendId); // 获取聊天记录
}