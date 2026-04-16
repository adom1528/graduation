package com.graduation.im.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.graduation.im.entity.User;
import org.apache.ibatis.annotations.*;


@Mapper
public interface UserMapper extends BaseMapper<User> {
    // 自动拥有insert, update, selectById, selectList 等所有方法（MyBatis Plus的特性）
}
