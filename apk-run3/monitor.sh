#!/bin/bash
# Monitor run 37905330999 until completed (max ~2.5h), then download artifact
RUN=37905330999
REPO=Wilykun/NarutoSenki-V2
DEST=~/workspace/narutosenki-v2/apk-run3
LOG=$DEST/monitor.log
MAX_ITER=37   # 37 * 240s = ~2.5h
i=0
echo "$(date -u +%FT%TZ) monitoring started for run $RUN" | tee -a $LOG
while [ $i -lt $MAX_ITER ]; do
  OUT=$(gh run view $RUN --repo $REPO --json status,conclusion 2>/dev/null)
  STATUS=$(echo "$OUT" | python3 -c "import json,sys; print(json.load(sys.stdin).get('status','?'))" 2>/dev/null)
  CONC=$(echo "$OUT" | python3 -c "import json,sys; print(json.load(sys.stdin).get('conclusion') or '-')" 2>/dev/null)
  echo "$(date -u +%FT%TZ) poll $i: status=$STATUS conclusion=$CONC" | tee -a $LOG
  if [ "$STATUS" = "completed" ]; then
    echo "COMPLETED conclusion=$CONC" | tee -a $LOG
    if [ "$CONC" = "success" ]; then
      gh run download $RUN --repo $REPO -D $DEST 2>&1 | tee -a $LOG
      echo "--- artifact files:" | tee -a $LOG
      find $DEST -type f | tee -a $LOG
    else
      gh run view $RUN --repo $REPO --log-failed > $DEST/failed.log 2>&1
      echo "failed log saved to $DEST/failed.log ($(wc -l < $DEST/failed.log) lines)" | tee -a $LOG
    fi
    exit 0
  fi
  i=$((i+1))
  sleep 240
done
echo "TIMEOUT after ~2.5h, run not completed" | tee -a $LOG
exit 2
