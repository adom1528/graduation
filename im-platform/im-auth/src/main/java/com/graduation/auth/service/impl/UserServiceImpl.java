package com.graduation.auth.service.impl;

import com.baomidou.mybatisplus.core.conditions.query.QueryWrapper;
import com.baomidou.mybatisplus.extension.service.impl.ServiceImpl;
import com.graduation.auth.common.JwtUtils;
import com.graduation.auth.entity.User;
import com.graduation.auth.mapper.UserMapper;
import com.graduation.auth.service.UserService;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

@Service
public class UserServiceImpl extends ServiceImpl<UserMapper, User> implements UserService {

    @Override
    public void register(String username, String password) {
        // 1. 检查用户名是否已存在
        QueryWrapper<User> queryWrapper = new QueryWrapper<>();
        queryWrapper.eq("username", username);
        if (count(queryWrapper) > 0) {
            throw new RuntimeException("用户名已存在");
        }

        // 2. 封装用户对象
        User user = new User();
        user.setUsername(username);
        // TODO: 这里后面要加 BCrypt 加密，现在先明文
        user.setPassword(password);
        user.setNickname("用户" + username); // 默认昵称

        // 3. 写入数据库
        save(user);
    }

    @Autowired
    private JwtUtils jwtUtils;
    @Override
    public String login(String username, String password) {
        // 1. 根据用户名查询用户
        QueryWrapper<User> queryWrapper = new QueryWrapper<>();
        queryWrapper.eq("username", username);
        User user = getOne(queryWrapper);

        // 2. 校验用户是否存在
        if (user == null) {
            throw new RuntimeException("用户不存在");
        }

        // 3. 校验密码 (目前是明文对比，后面我们再加加密)
        if (!user.getPassword().equals(password)) {
            throw new RuntimeException("密码错误");
        }

        // 4. 认证通过，生成 Token
        return jwtUtils.createToken(user.getId(), user.getUsername());
    }
}