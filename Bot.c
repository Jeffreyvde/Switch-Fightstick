#include "Bot.h"

typedef enum
{
    SYNC_CONTROLLER,
    FLY_TO_NURSERY, // X, A, A, A,  5 sec wait
    APPROACH_NPC, // <- x 7, ^ * 10, <- * 5
    PICK_SWAP_SLOT,
    SWAP_POKEMON,
    BREEDING_PREP,
    BREEDING,
    BREATHE,
    NEXT_ROUND
} State_t;

State_t state = SYNC_CONTROLLER;

#define ECHOES 2
int echoes = 0;
USB_JoystickReport_Input_t last_report;

int duration_count = 0;
int bufindex = 0;
int portsval = 0;
int swap_slot_number = 0;

static void reset_report(USB_JoystickReport_Input_t* const ReportData)
{
    memset(ReportData, 0, sizeof(USB_JoystickReport_Input_t));
    ReportData->LX = STICK_CENTER;
    ReportData->LY = STICK_CENTER;
    ReportData->RX = STICK_CENTER;
    ReportData->RY = STICK_CENTER;
    ReportData->HAT = HAT_CENTER;
}

static void take_action(action_t action, USB_JoystickReport_Input_t* const ReportData)
{
    switch (action)
    {
    case get_off_bike:
    case get_on_bike:
    case press_plus:
        ReportData->Button |= SWITCH_PLUS;
        break;

    case turn_left:
        ReportData->RX = STICK_MIN;
        break;

    case turn_right:
        ReportData->RX = STICK_MAX;
        break;

    case move_forward:
        ReportData->LY = STICK_MIN;
        break;
    case move_backward:
        ReportData->LY = STICK_MAX;
        break;

    case move_left:
        ReportData->LX = STICK_MIN;
        break;

    case move_right:
        ReportData->LX = STICK_MAX;
        break;

    case open_menu:
    case close_menu:
    case press_x:
        ReportData->Button |= SWITCH_X;
        break;

    case press_a:
        ReportData->Button |= SWITCH_A;
        break;

    case press_b:
        ReportData->Button |= SWITCH_B;
        break;

    case press_y:
        ReportData->Button |= SWITCH_Y;
        break;

    case press_down:
        ReportData->HAT |= HAT_BOTTOM;
        break;

    case press_up:
        ReportData->HAT |= HAT_TOP;
        break;

    case press_right:
        ReportData->HAT |= HAT_RIGHT;
        break;

    case press_left:
        ReportData->HAT |= HAT_LEFT;
        break;

    case reset_view_angle:
        ReportData->Button |= SWITCH_L;
        break;

    case circle_around:
        ReportData->LX = STICK_MAX;
        ReportData->RX = STICK_MAX;
        break;

    case hang:
        reset_report(ReportData);
        break;

    default:
        reset_report(ReportData);
        break;
    }
}

inline void do_steps(const command_t* steps, uint16_t steps_size, USB_JoystickReport_Input_t* const ReportData, State_t nextState, int add_swap_slot_number)
{
    take_action(steps[bufindex].action, ReportData);
    duration_count++;

    if (duration_count > steps[bufindex].duration)
    {
        bufindex++;
        duration_count = 0;
    }

    if (bufindex > steps_size - 1)
    {
        bufindex = 0;
        duration_count = 0;
        if (add_swap_slot_number)
        {
            swap_slot_number = (swap_slot_number + 1) % 5;
        }

        state = nextState;
        reset_report(ReportData);
    }
}

// Prepare the next report for the host.
void GetNextReport(USB_JoystickReport_Input_t* const ReportData)
{

    // Prepare an empty report
    reset_report(ReportData);

    // Repeat ECHOES times the last report
    if (echoes > 0)
    {
        memcpy(ReportData, &last_report, sizeof(USB_JoystickReport_Input_t));
        echoes--;
        return;
    }

    // States and moves management
    switch (state)
    {
    case SYNC_CONTROLLER:
        bufindex = 0;
        state = BREATHE;
        break;

    case BREATHE:
        do_steps(wake_up_hang, ARRAY_SIZE(wake_up_hang), ReportData, FLY_TO_NURSERY, 0);
        break;

    case FLY_TO_NURSERY:
        do_steps(fly_to_breading_steps, ARRAY_SIZE(fly_to_breading_steps), ReportData, APPROACH_NPC, 0);
        break;

    case APPROACH_NPC:
        do_steps(get_egg_steps, ARRAY_SIZE(get_egg_steps), ReportData, PICK_SWAP_SLOT, 0);
        break;

    case PICK_SWAP_SLOT:
        // do_steps(swap_slot[swap_slot_number], swap_slot_size[swap_slot_number], ReportData, SWAP_POKEMON, 1);
        take_action(swap_slot[swap_slot_number][bufindex].action, ReportData);
        duration_count++;

        if (duration_count > swap_slot[swap_slot_number][bufindex].duration)
        {
            bufindex++;
            duration_count = 0;
        }

        if (bufindex > swap_slot_size[swap_slot_number] - 1)
        {
            bufindex = 0;
            duration_count = 0;
            swap_slot_number = (swap_slot_number + 1) % 5;

            state = SWAP_POKEMON;

            reset_report(ReportData);
        }
        break;
    case SWAP_POKEMON:
        do_steps(swap_pokemon_steps, ARRAY_SIZE(swap_pokemon_steps), ReportData, BREEDING_PREP, 0);
        break;

    case BREEDING_PREP:
        do_steps(breeding_prep_steps, ARRAY_SIZE(breeding_prep_steps), ReportData, BREEDING, 0);
        break;

    case BREEDING:
        take_action(circle_around, ReportData);
        duration_count++;

        if (duration_count % 100 >= 0 && duration_count % 100 <= 5)
        {
            take_action(press_a, ReportData);
        }

        if (duration_count > breeding_duration - 1)
        {
            duration_count = 0;
            bufindex = 0;

            state = NEXT_ROUND;
        }
        break;

    case NEXT_ROUND:
        do_steps(next_round_steps, ARRAY_SIZE(next_round_steps), ReportData, FLY_TO_NURSERY, 0);
        break;
    }

    // Prepare to echo this report
    memcpy(&last_report, ReportData, sizeof(USB_JoystickReport_Input_t));
    echoes = ECHOES;
}
