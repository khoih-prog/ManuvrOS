/* Test driver for MCP3918 */
#include <inttypes.h>
#include <stdint.h>
#include <Platform/Peripherals/I2C/I2CAdapter.h>

#ifndef __SX8634_DRIVER_H__
#define __SX8634_DRIVER_H__

/* These are the i2c register definitions. */
#define SX8634_REG_IRQ_SRC                  0x00  // Read-only
#define SX8634_REG_CAP_STAT_MSB             0x01  // Read-only
#define SX8634_REG_CAP_STAT_LSB             0x02  // Read-only
#define SX8634_REG_SLIDER_POS_MSB           0x03  // Read-only
#define SX8634_REG_SLIDER_POS_LSB           0x04  // Read-only
#define SX8634_REG_RESERVED_0               0x05  // Read-only
#define SX8634_REG_RESERVED_1               0x06  // Read-only
#define SX8634_REG_GPI_STAT                 0x07  // Read-only
#define SX8634_REG_SPM_STAT                 0x08  // Read-only
#define SX8634_REG_COMP_OP_MODE             0x09  //
#define SX8634_REG_GPO_CTRL                 0x0A  //
#define SX8634_REG_GPP_PIN_ID               0x0B  //
#define SX8634_REG_GPP_INTENSITY            0x0C  //
#define SX8634_REG_SPM_CONFIG               0x0D  //
#define SX8634_REG_SPM_BASE_ADDR            0x0E  //
#define SX8634_REG_RESERVED_2               0x0F  //
#define SX8634_REG_SPM_KEY_MSB              0xAC  //
#define SX8634_REG_SPM_KEY_LSB              0xAD  //
#define SX8634_REG_SOFT_RESET               0xB1  //

/*
* These are the SPM register definitions.
* NOTE: In order to gain access to this data, the SPM needs to be opened. While
*   the SPM is open, the chip does not respond to touch.
*/
#define SX8634_SPM_RESERVED_00              0x00
#define SX8634_SPM_RESERVED_01              0x01
#define SX8634_SPM_RESERVED_02              0x02
#define SX8634_SPM_RESERVED_03              0x03
#define SX8634_SPM_I2C_ADDR                 0x04
#define SX8634_SPM_ACTIVE_SCAN_PERIOD       0x05
#define SX8634_SPM_DOZE_SCAN_PERIOD         0x06
#define SX8634_SPM_PASSIVE_TIMER            0x07
#define SX8634_SPM_RESERVED_04              0x08
#define SX8634_SPM_CAP_MODE_MISC            0x09
#define SX8634_SPM_CAP_MODE_11_8            0x0A
#define SX8634_SPM_CAP_MODE_7_4             0x0B
#define SX8634_SPM_CAP_MODE_3_0             0x0C
#define SX8634_SPM_CAP_SNSITIVTY_0_1        0x0D
#define SX8634_SPM_CAP_SNSITIVTY_2_3        0x0E
#define SX8634_SPM_CAP_SNSITIVTY_4_5        0x0F
#define SX8634_SPM_CAP_SNSITIVTY_6_7        0x10
#define SX8634_SPM_CAP_SNSITIVTY_8_9        0x11
#define SX8634_SPM_CAP_SNSITIVTY_10_11      0x12
#define SX8634_SPM_CAP_THRESH_0             0x13
#define SX8634_SPM_CAP_THRESH_1             0x14
#define SX8634_SPM_CAP_THRESH_2             0x15
#define SX8634_SPM_CAP_THRESH_3             0x16
#define SX8634_SPM_CAP_THRESH_4             0x17
#define SX8634_SPM_CAP_THRESH_5             0x18
#define SX8634_SPM_CAP_THRESH_6             0x19
#define SX8634_SPM_CAP_THRESH_7             0x1A
#define SX8634_SPM_CAP_THRESH_8             0x1B
#define SX8634_SPM_CAP_THRESH_9             0x1C
#define SX8634_SPM_CAP_THRESH_10            0x1D
#define SX8634_SPM_CAP_THRESH_11            0x1E
#define SX8634_SPM_CAP_PER_COMP             0x1F
#define SX8634_SPM_RESERVED_05              0x20
#define SX8634_SPM_BTN_CONFIG               0x21
#define SX8634_SPM_BTN_AVG_THRESH           0x22
#define SX8634_SPM_BTN_COMP_NEG_THRESH      0x23
#define SX8634_SPM_BTN_COMP_NEG_CNT_MAX     0x24
#define SX8634_SPM_BTN_HYSTERESIS           0x25
#define SX8634_SPM_BTN_STUCK_AT_TIMEOUT     0x26
#define SX8634_SPM_SLD_SLD_CONFIG           0x27
#define SX8634_SPM_SLD_STUCK_AT_TIMEOUT     0x28
#define SX8634_SPM_SLD_HYSTERESIS           0x29
#define SX8634_SPM_RESERVED_06              0x2A
#define SX8634_SPM_SLD_NORM_MSB             0x2B
#define SX8634_SPM_SLD_NORM_LSB             0x2C
#define SX8634_SPM_SLD_AVG_THRESHOLD        0x2D
#define SX8634_SPM_SLD_COMP_NEG_THRESH      0x2E
#define SX8634_SPM_SLD_COMP_NEG_CNT_MAX     0x2F
#define SX8634_SPM_SLD_MOVE_THRESHOLD       0x30
#define SX8634_SPM_RESERVED_07              0x31
#define SX8634_SPM_RESERVED_08              0x32
#define SX8634_SPM_MAP_WAKEUP_SIZE          0x33
#define SX8634_SPM_MAP_WAKEUP_VALUE_0       0x34
#define SX8634_SPM_MAP_WAKEUP_VALUE_1       0x35
#define SX8634_SPM_MAP_WAKEUP_VALUE_2       0x36
#define SX8634_SPM_MAP_AUTOLIGHT_0          0x37
#define SX8634_SPM_MAP_AUTOLIGHT_1          0x38
#define SX8634_SPM_MAP_AUTOLIGHT_2          0x39
#define SX8634_SPM_MAP_AUTOLIGHT_3          0x3A
#define SX8634_SPM_MAP_AUTOLIGHT_GRP_0_MSB  0x3B
#define SX8634_SPM_MAP_AUTOLIGHT_GRP_0_LSB  0x3C
#define SX8634_SPM_MAP_AUTOLIGHT_GRP_1_MSB  0x3D
#define SX8634_SPM_MAP_AUTOLIGHT_GRP_1_LSB  0x3E
#define SX8634_SPM_MAP_SEGMENT_HYSTERESIS   0x3F
#define SX8634_SPM_GPIO_7_4                 0x40
#define SX8634_SPM_GPIO_3_0                 0x41
#define SX8634_SPM_GPIO_OUT_PWR_UP          0x42
#define SX8634_SPM_GPIO_AUTOLIGHT           0x43
#define SX8634_SPM_GPIO_POLARITY            0x44
#define SX8634_SPM_GPIO_INTENSITY_ON_0      0x45
#define SX8634_SPM_GPIO_INTENSITY_ON_1      0x46
#define SX8634_SPM_GPIO_INTENSITY_ON_2      0x47
#define SX8634_SPM_GPIO_INTENSITY_ON_3      0x48
#define SX8634_SPM_GPIO_INTENSITY_ON_4      0x49
#define SX8634_SPM_GPIO_INTENSITY_ON_5      0x4A
#define SX8634_SPM_GPIO_INTENSITY_ON_6      0x4B
#define SX8634_SPM_GPIO_INTENSITY_ON_7      0x4C
#define SX8634_SPM_GPIO_INTENSITY_OFF_0     0x4D
#define SX8634_SPM_GPIO_INTENSITY_OFF_1     0x4E
#define SX8634_SPM_GPIO_INTENSITY_OFF_2     0x4F
#define SX8634_SPM_GPIO_INTENSITY_OFF_3     0x50
#define SX8634_SPM_GPIO_INTENSITY_OFF_4     0x51
#define SX8634_SPM_GPIO_INTENSITY_OFF_5     0x52
#define SX8634_SPM_GPIO_INTENSITY_OFF_6     0x53
#define SX8634_SPM_GPIO_INTENSITY_OFF_7     0x54
#define SX8634_SPM_RESERVED_09              0x55
#define SX8634_SPM_GPIO_FUNCTION            0x56
#define SX8634_SPM_GPIO_INC_FACTOR          0x57
#define SX8634_SPM_GPIO_DEC_FACTOR          0x58
#define SX8634_SPM_GPIO_INC_TIME_7_6        0x59
#define SX8634_SPM_GPIO_INC_TIME_5_4        0x5A
#define SX8634_SPM_GPIO_INC_TIME_3_2        0x5B
#define SX8634_SPM_GPIO_INC_TIME_1_0        0x5C
#define SX8634_SPM_GPIO_DEC_TIME_7_6        0x5D
#define SX8634_SPM_GPIO_DEC_TIME_5_4        0x5E
#define SX8634_SPM_GPIO_DEC_TIME_3_2        0x5F
#define SX8634_SPM_GPIO_DEC_TIME_1_0        0x60
#define SX8634_SPM_GPIO_OFF_DELAY_7_6       0x61
#define SX8634_SPM_GPIO_OFF_DELAY_5_4       0x62
#define SX8634_SPM_GPIO_OFF_DELAY_3_2       0x63
#define SX8634_SPM_GPIO_OFF_DELAY_1_0       0x64
#define SX8634_SPM_GPIO_PULL_UP_DOWN_7_4    0x65
#define SX8634_SPM_GPIO_PULL_UP_DOWN_3_0    0x66
#define SX8634_SPM_GPIO_INTERRUPT_7_4       0x67
#define SX8634_SPM_GPIO_INTERRUPT_3_0       0x68
#define SX8634_SPM_GPIO_DEBOUNCE            0x69
#define SX8634_SPM_RESERVED_0A              0x6A
#define SX8634_SPM_RESERVED_0B              0x6B
#define SX8634_SPM_RESERVED_0C              0x6C
#define SX8634_SPM_RESERVED_0D              0x6D
#define SX8634_SPM_RESERVED_0E              0x6E
#define SX8634_SPM_RESERVED_0F              0x6F
#define SX8634_SPM_CAP_PROX_ENABLE          0x70
#define SX8634_SPM_RESERVED_10              0x71
#define SX8634_SPM_RESERVED_11              0x72
#define SX8634_SPM_RESERVED_12              0x73
#define SX8634_SPM_RESERVED_13              0x74
#define SX8634_SPM_RESERVED_14              0x75
#define SX8634_SPM_RESERVED_15              0x76
#define SX8634_SPM_RESERVED_16              0x77
#define SX8634_SPM_RESERVED_17              0x78
#define SX8634_SPM_RESERVED_18              0x79
#define SX8634_SPM_RESERVED_19              0x7A
#define SX8634_SPM_RESERVED_1A              0x7B
#define SX8634_SPM_RESERVED_1B              0x7C
#define SX8634_SPM_RESERVED_1C              0x7D
#define SX8634_SPM_RESERVED_1D              0x7E
#define SX8634_SPM_SPM_CRC                  0x7F

/* Flags that help us track states */
#define SX8634_FLAG_PING_IN_FLIGHT        0x0001
#define SX8634_FLAG_DEV_FOUND             0x0002
#define SX8634_FLAG_IRQ_INHIBIT           0x0004
#define SX8634_FLAG_SPM_DIRTY             0x0008
#define SX8634_FLAG_SPM_SHADOWED          0x0010
#define SX8634_FLAG_COMPENSATING          0x0020
#define SX8634_FLAG_CONF_IS_NVM           0x0040
#define SX8634_FLAG_SLIDER_TOUCHED        0x0080
#define SX8634_FLAG_SLIDER_MOVE_DOWN      0x0100
#define SX8634_FLAG_SLIDER_MOVE_UP        0x0200
#define SX8634_FLAG_SPM_WRITABLE          0x0400
#define SX8634_FLAG_SPM_OPEN              0x0800


#define SX8634_DEFAULT_I2C_ADDR      0x2B


enum class SX8634OpMode : uint8_t {
  ACTIVE      = 0,
  DOZE        = 1,
  SLEEP       = 2,
  RESERVED    = 3
};

enum class SX8634_FSM : uint8_t {
  NO_INIT = 0,
  RESETTING,
  SPM_READ,
  SPM_WRITE,
  READY,
  NVM_BURN,
  NVM_VERIFY
};


class SX8634GPIOConf {
  public:
    // GPIOMode::ANALOG_OUT will be construed as the PWM mode.
    SX8634GPIOConf(GPIOMode mode, uint8_t boot_val);

  private:
    GPIOMode _mode;
    uint8_t  _default_val;
};


/*******************************************************************************
* Options object
*******************************************************************************/

/**
* Set pin def to 255 to mark it as unused.
*/
class SX8634Opts {
  public:
    const uint8_t  i2c_addr;
    const uint8_t  reset_pin;
    const uint8_t  irq_pin;
    const uint8_t* conf;

    /** Copy constructor. */
    SX8634Opts(const SX8634Opts* o) :
      i2c_addr(o->i2c_addr),
      reset_pin(o->reset_pin),
      irq_pin(o->irq_pin),
      conf(o->conf) {};



    /**
    * Constructor that accepts existing chip configuration. If the build
    *   includes the ability to burn the NVM, or wants the configuration
    *   for an already configured chip to be taken on faith, then this is
    *   the only valid constructor.
    *
    * @param address
    * @param reset pin
    * @param irq pin
    * @param Initial flags
    */
    SX8634Opts(
      uint8_t addr,
      uint8_t reset,
      uint8_t irq,
      const uint8_t* cdata
    ) :
      i2c_addr(addr),
      reset_pin(reset),
      irq_pin(irq),
      conf(cdata) {};


    #if !defined(CONFIG_SX8634_PROVISIONING) & !defined(CONFIG_SX8634_CONFIG_ON_FAITH)
    /**
    * Constructor that causes the driver to configure itself according to the
    *   contents of the SX8634 SPM. This will cause a period of delay on boot
    *   which might be obnoxious for some applications.
    *
    * @param address
    * @param reset pin
    * @param irq pin
    */
    SX8634Opts(
      uint8_t addr,
      uint8_t reset,
      uint8_t irq
    ) :
      i2c_addr(addr),
      reset_pin(reset),
      irq_pin(irq),
      conf(nullptr) {};
    #endif   // !CONFIG_SX8634_PROVISIONING & !CONFIG_SX8634_CONFIG_ON_FAITH

    bool haveResetPin() const {   return (255 != reset_pin);   };
    bool haveIRQPin() const {     return (255 != irq_pin);     };


  private:
};



/*******************************************************************************
*
*******************************************************************************/


class SX8634 : public I2CDevice {
  public:
    SX8634(const SX8634Opts*);
    ~SX8634();

    int8_t  reset();
    int8_t  init();

    /* Overrides from I2CDevice... */
    int8_t io_op_callahead(BusOp*);
    int8_t io_op_callback(BusOp*);
    void printDebug(StringBuilder*);

    int8_t setMode(SX8634OpMode);
    inline SX8634OpMode operationalMode() {  return _mode; };

    inline bool deviceFound() {  return _sx8634_flag(SX8634_FLAG_DEV_FOUND);  };
    inline bool buttonPressed(uint8_t i) {  return ((_buttons >> i) & 0x01);  };
    inline bool buttonReleased(uint8_t i) { return !((_buttons >> i) & 0x01); };

    /* GPIO functions */
    int8_t  setGPIOMode(uint8_t pin, GPIOMode);
    GPIOMode getGPIOMode(uint8_t pin);
    int8_t  setGPOValue(uint8_t pin, uint8_t value);
    uint8_t getGPIOValue(uint8_t pin);


    int8_t read_irq_registers();

    #if defined(CONFIG_SX8634_PROVISIONING)
      int8_t  burn_nvm();
    #endif  // CONFIG_SX8634_PROVISIONING

    inline void ping() { _ping_device(); };


  private:
    const SX8634Opts _opts;
    uint16_t _flags         = 0;
    uint16_t _slider_val    = 0;
    uint16_t _buttons       = 0;
    SX8634OpMode _mode  = SX8634OpMode::RESERVED;
    SX8634_FSM   _fsm   = SX8634_FSM::NO_INIT;
    uint8_t  _compensations = 0;
    uint8_t  _nvm_burns     = 0;
    uint8_t  _gpi_levels    = 0;
    uint8_t  _gpo_levels[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    uint8_t  _registers[16];     // Register shadows
    uint8_t  _spm_shadow[128];   // SPM shadow

    /* Flag manipulation inlines */
    inline uint16_t _sx8634_flags() {                return _flags;            };
    inline bool _sx8634_flag(uint16_t _flag) {       return (_flags & _flag);  };
    inline void _sx8634_flip_flag(uint16_t _flag) {  _flags ^= _flag;          };
    inline void _sx8634_clear_flag(uint16_t _flag) { _flags &= ~_flag;         };
    inline void _sx8634_set_flag(uint16_t _flag) {   _flags |= _flag;          };
    inline void _sx8634_set_flag(uint16_t _flag, bool nu) {
      if (nu) _flags |= _flag;
      else    _flags &= ~_flag;
    };

    void     _send_button_event(uint8_t button, bool pushed);

    int8_t   _get_shadow_reg_mem_addr(uint8_t addr);
    uint8_t  _get_shadow_reg_val(uint8_t addr);
    void     _set_shadow_reg_val(uint8_t addr, uint8_t val);
    void     _set_shadow_reg_val(uint8_t addr, uint8_t* buf, uint8_t len);
    int8_t   _write_register(uint8_t addr, uint8_t val);

    /* SPM region functions */
    int8_t  _read_full_spm();           // Mirror the SPM into our shadow.
    int8_t  _write_full_spm();          // Mirror our shadow into the SPM.
    int8_t  _open_spm_access_r();       //
    int8_t  _open_spm_access_w();       //
    int8_t  _close_spm_access();        //
    int8_t  _read_block8(uint8_t idx);  // Read 8 bytes from the SPM.
    int8_t  _write_block8(uint8_t idx); // Write 8 bytes to the SPM.
    int8_t  _compare_config();
    int8_t  _copy_boot_gpo_values();

    int8_t  _wait_for_reset(uint);    // Will block until reset disasserts or times out.
    int8_t  _clear_registers();       // Wipe our shadows.
    int8_t  _start_compensation();    // Tell the sensor to run a compensation cycle.
    int8_t  _ll_pin_init();           // Platform GPIO config
    int8_t  _ping_device();           // Pings the device.

    int8_t _write_gpo_register(uint8_t pin, bool val);
    int8_t _write_pwm_value(uint8_t pin, uint8_t val);
    int8_t _process_gpi_change(uint8_t new_val);
    inline bool _is_valid_pin(uint8_t pin) {  return (pin < 8);  };

    inline void       _set_fsm_position(SX8634_FSM x) {  _fsm = x;        };
    inline SX8634_FSM _get_fsm_position() {              return _fsm;     };

    static const char* getModeStr(SX8634OpMode);
    static const char* getFSMStr(SX8634_FSM);
    static const char* getSMStr();
};

#endif  // __SX8634_DRIVER_H__
