SIGNOFF_SCHEMA_VERSION = 1

VISUAL_RESOLUTIONS = (
    "3840x2160",
    "2560x1440",
    "1920x1080",
    "1280x720",
)

SUBJECTIVE_CHECKS = (
    ("enemyReadability", "三类敌人的颜色、形状、动作与前摇声音能在首次遭遇时被区分。"),
    ("projectileEvasion", "瘴气弹路径和速度可读，纯移动躲避不依赖预知脚本。"),
    ("dodgeResponse", "残剑魂冲斩的预警窗口与空格闪避响应自然，未感到输入迟滞。"),
    ("recoveryWindow", "石甲重砸后摇清晰，玩家能凭画面判断输出窗口。"),
    ("feedbackHierarchy", "普攻、术法、受击、会心、护障与击败反馈层级清楚，不互相淹没。"),
    ("impactComfort", "镜头冲击和局部停顿在连续战斗中舒适，无眩晕或明显打断操作。"),
    ("headphoneMix", "使用耳机能仅凭声音区分三类敌袭，整体响度与密度舒适。"),
    ("hudResolutionCoverage", "HUD 在 3840×2160 及三个兼容分辨率下清晰，无关键遮挡或裁切。"),
    ("combatPacing", "约 90–150 秒首战节奏有攻防起伏，整体战斗值得继续游玩。"),
)

FINAL_RESULTS = ("PASS", "NEEDS_ADJUSTMENT")


def build_signoff_template(operator, objective_packet_name):
    return {
        "schema": SIGNOFF_SCHEMA_VERSION,
        "objectivePacket": objective_packet_name,
        "operator": operator or "",
        "date": "",
        "environment": {
            "inputDevice": "",
            "display": "",
            "audioDevice": "",
            "performanceNotes": "",
        },
        "checks": {
            check_id: {
                "prompt": prompt,
                "result": "PENDING",
                "notes": "",
            }
            for check_id, prompt in SUBJECTIVE_CHECKS
        },
        "visualEvidence": {
            resolution: {
                "result": "PENDING",
                "notes": "",
                "artifact": f"VisualEvidence/W11_{resolution}.png",
            }
            for resolution in VISUAL_RESOLUTIONS
        },
        "overallResult": "PENDING",
        "signature": "",
    }
