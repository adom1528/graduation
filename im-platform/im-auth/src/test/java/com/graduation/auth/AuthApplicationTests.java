package com.graduation.auth;

import com.graduation.auth.entity.User;
import com.graduation.auth.mapper.UserMapper;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.transaction.annotation.Transactional; // 引入这个包

import java.util.List;

@SpringBootTest
public class AuthApplicationTests { // 类名 public

    @Autowired
    private UserMapper userMapper;

    @Test
    //@Transactional  // <--- 加上这个神器！
    void testDatabase() {
        System.out.println("------ 开始数据库测试 ------");

        // 1. 插入（此时数据真的进入了数据库的事务缓冲区）
        User user = new User();
        user.setUsername("tom"); // 换个名字方便区分
        user.setPassword("123");
        user.setNickname("业务代码测试");
        int result = userMapper.insert(user);
        System.out.println("插入结果: " + result);

        // 2. 查询（能查到，因为是在同一个事务里）
        List<User> users = userMapper.selectList(null);
        System.out.println("当前用户数量: " + users.size());
        //System.out.println("这次测试不会回滚，数据库里应该有用户数据。");
        System.out.println("------ 测试结束，Spring 会自动回滚事务，清理数据 ------");
    }
}