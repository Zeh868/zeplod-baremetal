# motor_foc_sensorless E1 成熟度声明

**Component**: `motor_foc_sensorless`  
**Version**: 0.1  
**Backend**: float32  
**Platform**: native_sim only  
**Maturity**: E1（前期应用探索）

## 已验证

- 磁链观测器 + PLL 角度更新（K0 `bm_algo_flux_observer_step`）
- MTPA / 弱磁 id 参考修正（K0）
- 电流环 + SVPWM 编排（无编码器反馈路径）
- `test_motor_foc_sensorless.c` 冒烟

## 未验证

- 实机 HAL、ADC 极性、死区与采样窗口
- 静止定位、开环启动、观测器切换与失锁回退
- 低速/过零/顺风启动/反转
- WCET、过调制、参数温漂与母线跌落全包络

## 不宜用于

- 量产电调或无感启动产品（需 V2/I3 证据）
- 未声明硬件包络下的安全关键应用
