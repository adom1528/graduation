package com.graduation.auth.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.graduation.auth.entity.User;
import org.apache.ibatis.annotations.Mapper;

@Mapper
public interface UserMapper extends BaseMapper<User> {
    // 自动拥有insert, update, selectById, selectList 等所有方法（MyBatis Plus的特性）
}
