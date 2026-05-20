package com.graduation.im.dto.res; // 或者是 .vo 包

import lombok.Data;

@Data
public class LoginVO {
    private String token;
    private String nickname;
    private String avatar;
}