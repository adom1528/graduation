package com.graduation.im.service.impl;

import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import com.baomidou.mybatisplus.extension.service.impl.ServiceImpl;
import com.graduation.im.common.JwtUtils;
import com.graduation.im.dto.req.*;
import com.graduation.im.dto.res.LoginVO;
import com.graduation.im.entity.User;
import com.graduation.im.mapper.UserMapper;
import com.graduation.im.service.UserService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.security.crypto.bcrypt.BCryptPasswordEncoder;
import org.springframework.stereotype.Service;

@Service
public class UserServiceImpl extends ServiceImpl<UserMapper, User> implements UserService {
    private static final BCryptPasswordEncoder encoder = new BCryptPasswordEncoder();

    @Override
    public void register(RegisterRequest request) {
        // 1. 检查用户名是否已存在
        QueryWrapper<User> queryWrapper = new QueryWrapper<>();
        queryWrapper.eq("username", request.getUsername());
        if (count(queryWrapper) > 0) {
            throw new RuntimeException("用户名已存在");
        }

        // 2. 封装用户对象
        User user = new User();
        user.setUsername(request.getUsername());
        user.setPassword(encoder.encode(request.getPassword()));

        // 处理昵称：为空则给默认值
        String nickname = request.getNickname();
        user.setNickname(nickname == null || nickname.trim().isEmpty() ? "用户" + request.getUsername() : nickname);

        // 处理性别：如果前端没传，默认填 0 (保密)
        Integer sex = request.getSex();
        user.setSex(sex != null ? sex : 0);

        // 处理头像：极其关键的默认值设定
        String defaultAvatar = "http://localhost:19000/im-chat/mrtx.jpg";
        String customAvatar = request.getAvatar();

        // 如果前端传了自定义头像 URL，就用前端的；否则用 MinIO 里的默认图
        user.setAvatar(customAvatar != null && !customAvatar.trim().isEmpty() ? customAvatar : defaultAvatar);

        // 3. 写入数据库
        save(user);
    }

    @Autowired
    private JwtUtils jwtUtils;
    @Override
    public LoginVO login(String username, String password) {
        // 1. 根据用户名查询用户
        QueryWrapper<User> queryWrapper = new QueryWrapper<>();
        queryWrapper.eq("username", username);
        User user = getOne(queryWrapper);

        // 2. 校验用户是否存在
        if (user == null) {
            throw new RuntimeException("用户不存在");
        }

        // 3. 校验密码
        if (!encoder.matches(password, user.getPassword())) {
            throw new RuntimeException("密码错误");
        }

        // 4. 认证通过，生成 Token
        String token = jwtUtils.createToken(user.getId(), user.getUsername());

        // 打包用户视图对象返回
        LoginVO loginVO = new LoginVO();
        loginVO.setToken(token);
        loginVO.setNickname(user.getNickname());
        loginVO.setAvatar(user.getAvatar()); // 从数据库里拿出的 MinIO 真实头像地址

        return loginVO;
    }
}