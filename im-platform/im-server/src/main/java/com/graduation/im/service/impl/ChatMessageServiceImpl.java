package com.graduation.im.service.impl;
import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import com.baomidou.mybatisplus.extension.service.impl.ServiceImpl;
import com.graduation.im.entity.ChatMessage;
import com.graduation.im.mapper.ChatMessageMapper;
import com.graduation.im.service.ChatMessageService;
import org.springframework.stereotype.Service;
import java.util.List;

@Service
public class ChatMessageServiceImpl extends ServiceImpl<ChatMessageMapper, ChatMessage> implements ChatMessageService {
    @Override
    public void saveMessage(Long fromId, Long toId, int type, String content, String fileName ) {
        ChatMessage msg = new ChatMessage();
        msg.setFromUserId(fromId);
        msg.setToUserId(toId);
        msg.setContent(content);
        msg.setType(type); // 1代表单聊，2代表群聊，3代表离线/在线状态，4是图片，5是文件
        msg.setFileName(fileName);
        this.save(msg); // 自动生成雪花ID并落库！
    }
    @Override
    public void saveMessage(Long fromId, Long toId, int type, String content ) {
        ChatMessage msg = new ChatMessage();
        msg.setFromUserId(fromId);
        msg.setToUserId(toId);
        msg.setContent(content);
        msg.setType(type); // 1代表单聊，2代表群聊，3代表离线/在线状态，4是图片，5是文件
        this.save(msg); // 自动生成雪花ID并落库！
    }
    @Override
    public List<ChatMessage> getHistory(Long userId, Long friendId) {
        QueryWrapper<ChatMessage> wrapper = new QueryWrapper<>();
        // SQL ：拼装嵌套的 AND 和 OR，并按时间升序
        wrapper.and(i -> i.eq("from_user_id", userId).eq("to_user_id", friendId))
                .or(i -> i.eq("from_user_id", friendId).eq("to_user_id", userId))
                .orderByAsc("create_time");

        return this.list(wrapper);
    }
}